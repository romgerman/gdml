#include "diff.h"

#include <cassert>
#include <algorithm>

#include <godot_cpp/core/print_string.hpp>

namespace gdml {

void gdml::Differ::diff(
	std::vector<godot::Node*> &nodes_a,
	std::vector<godot::Node*> &nodes_b,
	godot::Node *parent_a,
	std::vector<DiffItem> &out_diff,
	std::map<godot::ObjectID, godot::ObjectID> &remaps
) {
	// Check if any node is currently exiting the tree by a transition and do not check it against anything

	std::vector<godot::Node *> nodes_a_filtered;
	
	for (godot::Node *n : nodes_a) {
		if (!n->has_meta(godot::StringName("__gdml_transition_is_exiting"))) {
			nodes_a_filtered.push_back(n);
		}
	}

	if (nodes_a_filtered.empty() && nodes_b.empty()) {
		return;
	}

	// myers diff algo
	// https://docs.moonbitlang.com/en/latest/example/myers-diff/myers-diff.html

	int32_t n = nodes_a_filtered.size();
	int32_t m = nodes_b.size();
	int32_t max = n + m;
	std::vector<int32_t> v(max * 2 + 1);
	std::vector<std::vector<int32_t>> history;
	int32_t offset = max; // offset to handle negative indices
	bool found = false;

	for (int32_t d = 0; d < max + 1; d++) {
		history.push_back(v);

		for (int32_t k = -d; k < d + 1; k += 2) {
			int32_t k_index = k + offset;
			int32_t x = 0;
			int32_t y = 0;
			
			// Determine from where the edit comes from
			// left = deletion, right = insertion
			if (k == -d || (k != d && v[k_index - 1] < v[k_index + 1])) {
				x = v[k_index + 1]; // insertion
			} else {
				x = v[k_index - 1] + 1; // deletion
			}

			y = x - k;

			while (x < n && y < m && is_node_equal(nodes_a_filtered[x], nodes_b[y])) {
				x++;
				y++;
			}

			v[k_index] = x;

			if (x >= n && y >= m) {
				found = true;
				break;
			}
		}

		if (found) {
			break;
		}
	}

	// extract the edits

	int32_t x = n;
	int32_t y = m;
	std::vector<DiffItem> diffs;
	std::vector<std::pair<godot::Node *, godot::Node *>> matches;

	for (int32_t d = history.size() - 1; d >= 0; d--) {
		std::vector<int32_t> &current_v = history[d];
		int32_t k = x - y;
		int32_t k_index = k + offset;
		int32_t prev_k = k;

		bool prev_vert = false;
		if (k == -d || (k != d && current_v[k_index - 1] < current_v[k_index + 1])) {
			prev_k++; // insertion
			prev_vert = true;
		} else {
			prev_k--; // deletion
			prev_vert = false;
		}

		int32_t prev_x = current_v[prev_k + offset];
		int32_t prev_y = prev_x - prev_k;

		while (x > prev_x && y > prev_y) {
			godot::Node *a = nodes_a_filtered[x - 1];
			godot::Node *b = nodes_b[y - 1];

			matches.push_back({ a, b });

			DiffItem item;
			item.type = DiffOp::UPDATE;
			item.node = b;
			item.old_node = a;
			diff_node_props(a, b, item.props);
			if (!item.props.empty()) {
				diffs.push_back(item);
			}

			x--;
			y--;
		}

		if (d == 0) {
			break;
		}

		if (prev_vert) {
			// insertion
			godot::Node *b = nodes_b[y - 1];
			//int32_t b_index = b->get_index() == 0xFFFFFFFF ? 0 : b->get_index();
			DiffItem item;
			item.type = DiffOp::INSERT;
			item.node = b;
			item.index = prev_y;
			item.parent = parent_a;
			diffs.push_back(item);
			y--;
		} else {
			// deletion
			godot::Node *a = nodes_a_filtered[x - 1];
			// int32_t a_index = a->get_index() == 0xFFFFFFFF ? 0 : a->get_index();
			DiffItem item;
			item.type = DiffOp::DELETE;
			item.node = a;
			item.index = prev_x;
			item.parent = parent_a;
			diffs.push_back(item);
			x--;
		}

		if (d > 0) {
			assert(x == prev_x && y == prev_y);
		}
	}

	std::vector<bool> ops_to_erase(diffs.size(), false);
	std::vector<DiffItem> moves;
	std::vector<DiffItem> move_updates;

	for (size_t i = 0; i < diffs.size(); i++) {
		if (diffs[i].type != DiffOp::DELETE) {
			continue;
		}

		for (size_t j = 0; j < diffs.size(); j++) {
			if (ops_to_erase[j] || diffs[j].type != DiffOp::INSERT) {
				continue;
			}

			if (is_node_equal(diffs[i].node, diffs[j].node)) {
				DiffItem item;
				item.type = DiffOp::MOVE;
				item.old_node = diffs[i].node;
				item.node = diffs[j].node;
				item.old_index = diffs[i].index;
				item.index = diffs[j].index;
				item.parent = diffs[i].parent;
				moves.push_back(item);
				
				matches.push_back({ item.old_node, item.node });
				DiffItem update_item;
				update_item.type = DiffOp::UPDATE;
				update_item.old_node = item.old_node;
				update_item.node = item.node;
				diff_node_props(item.old_node, item.node, update_item.props);
				if (!update_item.props.empty()) {
					move_updates.push_back(update_item);
				}

				ops_to_erase[i] = true;
				ops_to_erase[j] = true;
				break;
			}
		}
	}

	std::sort(moves.begin(), moves.end(), [](DiffItem &a, DiffItem &b) {
		return a.index < b.index;
	});

	// reverse the edits from backwards to forwards
	for (int32_t i = diffs.size() - 1; i >= 0; i--) {
		if (!ops_to_erase[i]) {
			out_diff.push_back(diffs[i]);
		}
	}

	out_diff.insert(out_diff.end(), move_updates.begin(), move_updates.end());
	out_diff.insert(out_diff.end(), moves.begin(), moves.end());
	
	for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
		std::vector<godot::Node*> children_a;
		std::vector<godot::Node*> children_b;

		for (godot::Variant item : it->first->get_children()) {
			children_a.push_back(godot::Object::cast_to<godot::Node>(item));
		}

		for (godot::Variant item : it->second->get_children()) {
			children_b.push_back(godot::Object::cast_to<godot::Node>(item));
		}

		remaps[(godot::ObjectID)it->second->get_instance_id()] = it->first->get_instance_id();
		diff(children_a, children_b, it->first, out_diff, remaps);
	}
}

bool Differ::is_node_equal(godot::Node *a, godot::Node *b) {
	static const godot::StringName ANCHOR_META = godot::StringName("__gdml_anchor");
	static const godot::StringName KEY_META = godot::StringName("__gdml_key");

	bool aa = a->has_meta(ANCHOR_META);
	bool ba = b->has_meta(ANCHOR_META);
	if (aa || ba) {
		return aa && ba;
	}
	if (a->get_class() != b->get_class()) {
		return false;
	}
	bool a_has_key = a->has_meta(KEY_META);
	bool b_has_key = b->has_meta(KEY_META);
	if (a_has_key || b_has_key) {
		return a_has_key && b_has_key && a->get_meta(KEY_META) == b->get_meta(KEY_META);
	} else {
		return true;
	}
}

void Differ::diff_node_props(godot::Node *a, godot::Node *b, std::vector<godot::StringName> &out_props) {
	static const godot::StringName DEFINED_PROPS_META = godot::StringName("__gdml_defined_props");

	if (a->has_meta(DEFINED_PROPS_META)) {
		for (godot::Variant item : (godot::Array)a->get_meta(DEFINED_PROPS_META)) {
			godot::String prop_key = item;
			if (!prop_key.contains("metadata/") && a->get(prop_key) != b->get(prop_key)) {
				out_props.push_back(prop_key);
			}
		}
	} else {
		for (godot::Variant item : a->get_property_list()) {
			godot::Dictionary prop_info = (godot::Dictionary)item;
			int64_t usage = (int64_t)prop_info["usage"];
			if (usage & godot::PropertyUsageFlags::PROPERTY_USAGE_STORAGE) {
				godot::StringName prop_key = prop_info["name"];
				if (!prop_key.contains("metadata/") && a->get(prop_key) != b->get(prop_key)) {
					out_props.push_back(prop_key);
				}
			}
		}
	}
}

} // namespace gdml

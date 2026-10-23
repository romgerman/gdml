#include "diff.h"

#include <cassert>
#include <map>
#include <algorithm>

#include <godot_cpp/core/print_string.hpp>

namespace gdml {

void gdml::Differ::diff(std::vector<godot::Node*> &nodes_a, std::vector<godot::Node*> &nodes_b, godot::Node *parent_a, std::vector<DiffItem> &out_diff) {
	// myers diff algo
	// https://docs.moonbitlang.com/en/latest/example/myers-diff/myers-diff.html

	int32_t n = nodes_a.size();
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

			while (x < n && y < m && is_node_equal(nodes_a[x], nodes_b[y])) {
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
			godot::Node *a = nodes_a[x - 1];
			godot::Node *b = nodes_b[y - 1];
			int32_t b_index = b->get_index() == 0xFFFFFFFF ? 0 : b->get_index();

			matches.push_back({ a, b });

			if (a->get_index() != b_index) {
				DiffItem item;
				item.type = DiffType::MOVE;
				item.node = a;
				item.index = b_index;
				item.parent = parent_a;
				diffs.push_back(item);
			} else {
				DiffItem item;
				item.type = DiffType::UPDATE;
				item.node = b;
				item.old_node = a;
				diff_node_props(a, b, item.props);
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
			int32_t b_index = b->get_index() == 0xFFFFFFFF ? 0 : b->get_index();
			DiffItem item;
			item.type = DiffType::INSERT;
			item.node = b;
			item.index = b_index;
			item.parent = parent_a;
			diffs.push_back(item);
			y--;
		} else {
			// deletion
			godot::Node *a = nodes_a[x - 1];
			DiffItem item;
			item.type = DiffType::DELETE;
			item.node = a;
			diffs.push_back(item);
			x--;
		}

		if (d > 0) {
			assert(x == prev_x && y == prev_y);
		}
	}

	// reverse the edits from backwards to forwards
	for (auto it = diffs.rbegin(); it != diffs.rend(); ++it) {
		out_diff.push_back(*it);
	}

	for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
		std::vector<godot::Node*> children_a;
		std::vector<godot::Node*> children_b;

		for (godot::Variant item : it->first->get_children()) {
			children_a.push_back(godot::Object::cast_to<godot::Node>(item));
		}

		for (godot::Variant item : it->second->get_children()) {
			children_b.push_back(godot::Object::cast_to<godot::Node>(item));
		}


		diff(children_a, children_b, it->first, out_diff);
	}
}

bool Differ::is_node_equal(godot::Node *a, godot::Node *b) {
	int32_t a_index = a->get_index() == 0xFFFFFFFF ? 0 : a->get_index();
	int32_t b_index = b->get_index() == 0xFFFFFFFF ? 0 : b->get_index();
	return a->get_class() == b->get_class() && a_index == b_index;
}

void Differ::diff_node_props(godot::Node *a, godot::Node *b, std::vector<godot::StringName> &out_props) {
	for (godot::Variant item : a->get_property_list()) {
		godot::Dictionary prop_info = (godot::Dictionary)item;
		if ((int64_t)prop_info["usage"] == 6) {
			godot::StringName prop_key = prop_info["name"];
			if (a->get(prop_key) != b->get(prop_key)) {
				out_props.push_back(prop_key);
			}
		}
	}
}

} // namespace gdml

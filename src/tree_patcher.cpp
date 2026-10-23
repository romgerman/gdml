#include "tree_patcher.h"

namespace gdml {

void gdml::TreePatcher::patch(godot::Node *node, const std::vector<DiffItem> &diff) {
	// deletions
	for (const DiffItem &item : diff) {
		if (item.type == DiffType::DELETE) {
			if (item.node) {
				item.node->queue_free();
			}
		}
	}

	// move and update
	for (const DiffItem &item : diff) {
		if (item.type == DiffType::MOVE) {
			if (item.node) {
				item.parent->move_child(item.node, item.index);
			}
		} else if (item.type == DiffType::UPDATE) {
			if (item.old_node && item.node) {
				for (godot::StringName prop_key : item.props) {
					item.old_node->set(prop_key, item.node->get(prop_key));
				}
			}
		}
	}

	// insertions
	for (const DiffItem &item : diff) {
		if (item.type == DiffType::INSERT) {
			if (item.parent && item.node) {
				if (item.node->get_parent()) {
					item.node->get_parent()->remove_child(item.node);
				}
				item.parent->add_child(item.node);
				item.parent->move_child(item.node, item.index);
			}
		}
	}
}

}

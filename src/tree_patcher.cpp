#include "tree_patcher.h"

#include "built-in/decorators/transition.hpp"
#include "built-in/decorators/transition_settings.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/tween.hpp>

namespace gdml {

void gdml::TreePatcher::patch(const std::vector<DiffItem> &diff) {
	// for (int32_t i = 0; i < diff.size(); i++) {
	// 	switch (diff[i].type)
	// 	{
	// 	case DiffOp::DELETE:
	// 		godot::print_line("DELETE: ", diff[i].node, " at ", diff[i].index);
	// 		break;
	// 	case DiffOp::INSERT:
	// 		godot::print_line("INSERT: ", diff[i].node, " at ", diff[i].index);
	// 		break;
	// 	case DiffOp::MOVE:
	// 		godot::print_line("MOVE: ", diff[i].old_node, " from ", diff[i].old_index, " to ", diff[i].index);
	// 		break;
	// 	case DiffOp::UPDATE:
	// 		godot::print_line("UPDATE: ", diff[i].node, " at ", diff[i].index);
	// 		break;
	// 	}
	// }

	// deletions
	for (const DiffItem &item : diff) {
		if (item.type == DiffOp::DELETE) {
			if (item.node && !item.node->has_meta(godot::StringName("__gdml_transition_is_exiting"))) {
				if (item.node->has_meta(godot::StringName("__gdml_transition"))
					&& item.node->has_meta(godot::StringName("__gdml_transition_out_tween_factory"))
					&& item.node->has_meta(godot::StringName("__gdml_transition_out_tween_settings"))
					&& godot::Object::cast_to<GDMLTransitionOptions>(item.node->get_meta(godot::StringName("__gdml_transition_out_tween_settings")))->get_enabled()
				) {
					item.node->set_meta(godot::StringName("__gdml_transition_is_exiting"), true);

					godot::Callable tween_factory = item.node->get_meta(godot::StringName("__gdml_transition_out_tween_factory"));
					godot::Ref<godot::Tween> tween = tween_factory.call(item.node);
					tween->connect(godot::StringName("finished"), callable_mp(item.node, &godot::Node::queue_free), godot::Object::CONNECT_ONE_SHOT);

					if (item.node->get_parent()) {
						item.node->get_parent()->move_child(item.node, item.index);
					}
				} else {
					item.node->queue_free();
				}
			}
		}
	}

	// insertions
	for (const DiffItem &item : diff) {
		if (item.type == DiffOp::INSERT) {
			if (item.parent && item.node) {
				if (item.node->get_parent()) {
					item.node->get_parent()->remove_child(item.node);
				}
				item.parent->add_child(item.node);
				item.parent->move_child(item.node, item.index);
			}
		}
	}

	for (const DiffItem &item : diff) {
		if (item.type == DiffOp::MOVE) {
			if (item.parent && item.node && item.old_node) {
				if (item.parent != item.old_node->get_parent()) {
					item.old_node->reparent(item.parent);
				}
				item.parent->move_child(item.old_node, item.index);
			}
		}
	}

	// update
	for (const DiffItem &item : diff) {
		if (item.type == DiffOp::UPDATE) {
			if (item.old_node && item.node) {
				for (godot::StringName prop_key : item.props) {
					item.old_node->set(prop_key, item.node->get(prop_key));
				}
				item.node->queue_free();
			}
		}
	}
}

} //namespace gdml

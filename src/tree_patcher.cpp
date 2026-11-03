#include "tree_patcher.h"

#include "built-in/decorators/transition.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/tween.hpp>

namespace gdml {

void gdml::TreePatcher::patch(const std::vector<DiffItem> &diff) {
	// deletions
	for (const DiffItem &item : diff) {
		if (item.type == DiffType::DELETE) {
			if (item.node && !item.node->has_meta(godot::StringName("__gdml_transition_is_exiting"))) {
				if (item.node->has_meta(godot::StringName("__gdml_transition"))
					&& item.node->get_meta(godot::StringName("__gdml_transition")) == godot::Variant(true)
					&& item.node->has_meta(godot::StringName("__gdml_transition_out_tween_factory"))) {
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

	// update
	for (const DiffItem &item : diff) {
		if (item.type == DiffType::UPDATE) {
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

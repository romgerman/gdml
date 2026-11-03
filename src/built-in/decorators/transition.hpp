#pragma once

#include "gdml_decorator.h"

#include <set>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/weak_ref.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

namespace gdml {

class GDMLResult;

class GDMLDecorator_transition : public GDMLDecorator {
	GDCLASS(GDMLDecorator_transition, GDMLDecorator)

	std::set<godot::ObjectID> transitioned_nodes_in;

protected:
	static void _bind_methods() {
		godot::ClassDB::bind_method(godot::D_METHOD("modify", "control", "args", "context"), &GDMLDecorator_transition::modify);
		godot::ClassDB::bind_method(godot::D_METHOD("get_decorator_name"), &GDMLDecorator_transition::get_decorator_name);
		godot::ClassDB::bind_method(godot::D_METHOD("before_render"), &GDMLDecorator_transition::before_render);
	}

public:
	void before_render() override {
		for (auto it = transitioned_nodes_in.begin(); it != transitioned_nodes_in.end();) {
			if (!godot::ObjectDB::get_instance(*it)) {
				it = transitioned_nodes_in.erase(it);
			} else {
				++it;
			}
		}
	}

	void modify(godot::Control *p_control, godot::Array p_args, godot::Ref<GDMLResult> p_context) override {
		if (p_args.size() > 1) {
			godot::String transition_type = p_args[0];
			if (p_args.size() > 2) {
				bool is_enabled = (bool)p_args[2];
				if (!is_enabled) {
					p_control->set_meta(godot::StringName("__gdml_transition"), false);
					return;
				}
			}
			if (transition_type == "enter") {
				if (transitioned_nodes_in.find((godot::ObjectID)p_control->get_instance_id()) != transitioned_nodes_in.end()) {
					return;
				}
				godot::Callable tween_factory = p_args[1];
				if (tween_factory.is_valid()) {
					p_control->set_meta(godot::StringName("__gdml_transition"), true);
					p_control->set_meta(godot::StringName("__gdml_transition_in_tween_factory"), tween_factory);
					
					tween_factory.call_deferred(p_control);
					
					transitioned_nodes_in.emplace((godot::ObjectID)p_control->get_instance_id());
				}
			} else if (transition_type == "exit") {
				godot::Callable tween_factory = p_args[1];
				if (tween_factory.is_valid()) {
					p_control->set_meta(godot::StringName("__gdml_transition"), true);
					p_control->set_meta(godot::StringName("__gdml_transition_out_tween_factory"), tween_factory);
				}
			}
		}
	}

	godot::StringName get_decorator_name() override {
		return godot::StringName("transition");
	}
};

}

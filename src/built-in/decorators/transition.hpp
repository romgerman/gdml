#pragma once

#include "gdml_decorator.h"
#include "transition_settings.hpp"

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
		for (godot::Variant arg_item : p_args) {
			godot::Ref<GDMLTransitionEnter> enter_settings = Object::cast_to<GDMLTransitionEnter>(arg_item);
			if (enter_settings.is_valid()) {
				if (!enter_settings->get_enabled()) {
					transitioned_nodes_in.emplace((godot::ObjectID)p_control->get_instance_id());
					continue;
				}
				if (transitioned_nodes_in.find((godot::ObjectID)p_control->get_instance_id()) != transitioned_nodes_in.end()) {
					continue;
				}
				godot::Callable tween_factory = enter_settings->get_tween_factory();
				if (tween_factory.is_valid()) {
					p_control->set_meta(godot::StringName("__gdml_transition"), true);
					p_control->set_meta(godot::StringName("__gdml_transition_in_tween_factory"), tween_factory);
					p_control->set_meta(godot::StringName("__gdml_transition_in_tween_settings"), enter_settings);
					
					tween_factory.call_deferred(p_control);
					
					transitioned_nodes_in.emplace((godot::ObjectID)p_control->get_instance_id());
				}
				continue;
			}

			godot::Ref<GDMLTransitionExit> exit_settings = Object::cast_to<GDMLTransitionExit>(arg_item);
			if (exit_settings.is_valid()) {
				if (!exit_settings->get_enabled()) {
					continue;
				}
				godot::Callable tween_factory = exit_settings->get_tween_factory();
				if (tween_factory.is_valid()) {
					p_control->set_meta(godot::StringName("__gdml_transition"), true);
					p_control->set_meta(godot::StringName("__gdml_transition_out_tween_factory"), tween_factory);
					p_control->set_meta(godot::StringName("__gdml_transition_out_tween_settings"), exit_settings);
				}

				continue;
			}

			godot::print_error("Unsupported argument type in transition decorator");
		}
	}

	godot::StringName get_decorator_name() override {
		return godot::StringName("transition");
	}
};

}

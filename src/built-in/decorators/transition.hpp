#pragma once

#include "gdml_decorator.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

namespace gdml {

class GDMLResult;

class GDMLDecorator_transition : public GDMLDecorator {
	GDCLASS(GDMLDecorator_transition, GDMLDecorator)

protected:
	static void _bind_methods() {
		godot::ClassDB::bind_method(godot::D_METHOD("modify", "control", "args", "context"), &GDMLDecorator_transition::modify);
		godot::ClassDB::bind_method(godot::D_METHOD("get_decorator_name"), &GDMLDecorator_transition::get_decorator_name);
	}

public:
	void modify(godot::Control *p_control, godot::Array p_args, godot::Ref<GDMLResult> p_context) override {
		p_control->set_meta(godot::StringName("__gdml_transition"), true);
	}

	godot::StringName get_decorator_name() override {
		return godot::StringName("transition");
	}
};

}

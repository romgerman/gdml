#include "gdml_decorator.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace gdml {

void GDMLDecorator::_bind_methods() {
	GDVIRTUAL_BIND(before_render);
	GDVIRTUAL_BIND(modify, "control", "args", "context");
	GDVIRTUAL_BIND(modify_resource, "resource", "args");
	GDVIRTUAL_BIND(get_decorator_name);
}

void GDMLDecorator::before_render() {
	if (GDVIRTUAL_IS_OVERRIDDEN(before_render)) {
		GDVIRTUAL_CALL(before_render);
	}
}

void GDMLDecorator::modify(Control *p_control, Array p_args, godot::Ref<GDMLResult> p_context) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify)) {
		GDVIRTUAL_CALL(modify, p_control, p_args, p_context);
	}
}

Variant GDMLDecorator::modify_resource(Variant *p_resource, Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify_resource)) {
		Variant out;
		GDVIRTUAL_CALL(modify_resource, p_resource, p_args, out);
		return out;
	}
	return p_resource;
}

godot::StringName GDMLDecorator::get_decorator_name() {
	if (GDVIRTUAL_IS_OVERRIDDEN(get_decorator_name)) {
		Variant out;
		GDVIRTUAL_CALL(get_decorator_name, out);
		return out;
	}
	return godot::StringName();
}

}
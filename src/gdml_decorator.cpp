#include "gdml_decorator.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace gdml {

void GDMLDecorator::_bind_methods() {
	GDVIRTUAL_BIND(modify, "control", "args");
	GDVIRTUAL_BIND(modify_resource, "resource", "args");
}

void GDMLDecorator::modify(Control *p_control, Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify)) {
		GDVIRTUAL_CALL(modify, p_control, p_args);
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

}
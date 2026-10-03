#include "gdml_decorator.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace gdml {

void GDMLDecorator::_bind_methods() {
	GDVIRTUAL_BIND(modify);
	GDVIRTUAL_BIND(modify_resource);
}

void GDMLDecorator::modify(godot::Control *p_control, godot::Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify)) {
		GDVIRTUAL_CALL(modify, p_control, p_args);
	}
}

godot::Variant GDMLDecorator::modify_resource(godot::Variant *p_resource, godot::Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify_resource)) {
		Variant out;
		GDVIRTUAL_CALL(modify_resource, p_resource, p_args, out);
		return out;
	}
	return p_resource;
}

}
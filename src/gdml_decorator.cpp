#include "gdml_decorator.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace gdml {

void GDMLDecorator::_bind_methods() {
	GDVIRTUAL_BIND(modify);
}

void GDMLDecorator::modify(godot::Control *p_control, godot::Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(modify)) {
		GDVIRTUAL_CALL(modify, p_control, p_args);
	}
}

}
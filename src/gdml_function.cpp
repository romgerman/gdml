#include "gdml_function.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace gdml {

void GDMLFunction::_bind_methods() {
	GDVIRTUAL_BIND(execute, "args");
	GDVIRTUAL_BIND(get_name);
}

godot::Variant GDMLFunction::execute(godot::Array p_args) {
	if (GDVIRTUAL_IS_OVERRIDDEN(execute)) {
		Variant out;
		GDVIRTUAL_CALL(execute, p_args, out);
		return out;
	}
	return Variant();
}

godot::StringName GDMLFunction::get_name() {
	if (GDVIRTUAL_IS_OVERRIDDEN(get_name)) {
		Variant out;
		GDVIRTUAL_CALL(get_name, out);
		return out;
	}
	return godot::StringName();
}

}

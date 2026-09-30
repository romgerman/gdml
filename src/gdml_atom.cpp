#include "gdml_atom.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void reactive::GDMLAtom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_value"), &GDMLAtom::get_value);
	ClassDB::bind_method(D_METHOD("set_value", "_value"), &GDMLAtom::set_value);

	ADD_PROPERTY(
		PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT & ~PROPERTY_USAGE_EDITOR),
		"set_value",
		"get_value"
	);

	ADD_SIGNAL(MethodInfo("changed"));
}

void reactive::GDMLAtom::create(godot::StringName p_name, godot::Variant p_initial_value) {
	m_name = p_name;
	m_value = p_initial_value;
}

void reactive::GDMLAtom::set_value(godot::Variant p_value) {
	m_value = p_value;
	emit_signal("changed");
}

godot::Variant reactive::GDMLAtom::get_value() {
	return m_value;
}

godot::StringName reactive::GDMLAtom::get_name() {
	return m_name;
}

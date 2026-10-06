#include "gdml_global.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void global::gdml::_bind_methods() {
	ClassDB::bind_static_method("gdml", D_METHOD("atom", "name", "initial_value"), &global::gdml::create_atom, DEFVAL(Variant()));
}

reactive::GDMLAtom *global::gdml::create_atom(StringName p_name, Variant p_initial_value) {
	reactive::GDMLAtom *atom = memnew(reactive::GDMLAtom);
	atom->create(p_name, p_initial_value);
	return atom;
}

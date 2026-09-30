#include "gdml_global.h"

#include <godot_cpp/core/class_db.hpp>

void global::gdml::_bind_methods() {
	godot::ClassDB::bind_static_method("gdml", godot::D_METHOD("atom", "name", "initial_value"), &global::gdml::create_atom, DEFVAL(godot::Variant()));
	godot::ClassDB::bind_static_method("gdml", godot::D_METHOD("use_effect", "p_fn"), &global::gdml::use_effect);
}

reactive::GDMLAtom *global::gdml::create_atom(godot::StringName p_name, godot::Variant p_initial_value) {
	reactive::GDMLAtom *atom = memnew(reactive::GDMLAtom);
	atom->create(p_name, p_initial_value);
	return atom;
}

void global::gdml::use_effect(godot::Callable fn) {

}

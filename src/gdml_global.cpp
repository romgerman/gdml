#include "gdml_global.h"

#include <godot_cpp/core/class_db.hpp>

void global::gdml::_bind_methods() {
	godot::ClassDB::bind_static_method("gdml", godot::D_METHOD("atom"), &global::gdml::create_atom);
}

reactive::GdmlAtom *global::gdml::create_atom() {
	return memnew(reactive::GdmlAtom);
}

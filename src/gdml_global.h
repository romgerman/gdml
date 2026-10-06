#pragma once

#include "gdml_atom.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace global {

class gdml : public godot::Object {
	GDCLASS(gdml, godot::Object);

protected:
	static void _bind_methods();

public:
	static reactive::GDMLAtom *create_atom(godot::StringName p_name, godot::Variant p_initial_value = godot::Variant());
};

} //namespace global

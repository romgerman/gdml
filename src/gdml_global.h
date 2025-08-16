#pragma once

#include "gdml_atom.h"

#include <godot_cpp/classes/object.hpp>

namespace global {

class gdml : public godot::Object {
	GDCLASS(gdml, godot::Object);

protected:
	static void _bind_methods();

public:
	static reactive::GdmlAtom *create_atom();
};

} //namespace global

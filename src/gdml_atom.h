#pragma once

#include <godot_cpp/classes/object.hpp>

namespace reactive {

class GdmlAtom : public godot::Object {
	GDCLASS(GdmlAtom, Object);

protected:
    static void _bind_methods();
};

}

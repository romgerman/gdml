#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace gdml {

class GDMLFunction : public godot::RefCounted {
	GDCLASS(GDMLFunction, godot::RefCounted)

protected:
	static void _bind_methods();

public:
	virtual godot::Variant execute(godot::Array p_args);
	virtual godot::StringName get_name();

	GDVIRTUAL1R(godot::Variant, execute, godot::Array);
	GDVIRTUAL0R(godot::Variant, get_name);
};

}

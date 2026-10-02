#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

using namespace godot;

namespace gdml {

class GDMLDecorator : public godot::Object {
	GDCLASS(GDMLDecorator, godot::Object)

protected:
	static void _bind_methods();

public:
	virtual void modify(godot::Control *p_control, godot::Array p_args);

	GDVIRTUAL2(modify, godot::Variant, godot::Array);
};

}

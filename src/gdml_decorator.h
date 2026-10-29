#pragma once

#include "render/gdml_renderer.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace gdml {

class GDMLDecorator : public godot::RefCounted {
	GDCLASS(GDMLDecorator, godot::RefCounted)

protected:
	static void _bind_methods();

public:
	virtual void modify(godot::Control *p_control, godot::Array p_args, godot::Ref<GDMLResult> p_context);
	virtual godot::Variant modify_resource(godot::Variant *p_resource, godot::Array p_args);
	virtual godot::StringName get_decorator_name();

	GDVIRTUAL3(modify, godot::Variant, godot::Array, godot::Ref<GDMLResult>);
	GDVIRTUAL2R(godot::Variant, modify_resource, godot::Variant, godot::Array);
	GDVIRTUAL0R(godot::Variant, get_decorator_name);
};

}

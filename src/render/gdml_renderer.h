#pragma once

#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"
#include "gdml_atom.h"

#include <map>
#include <memory>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

class GDMLResult : public RefCounted {
	GDCLASS(GDMLResult, RefCounted)

protected:
	static void _bind_methods();

public:
	godot::Array nodes;
	godot::Dictionary variables;

	godot::Array get_nodes();

	godot::Dictionary get_variable_list();
	godot::Ref<reactive::GDMLAtom> get_variable(godot::String &p_name);
};

class GdmlRenderer : public RefCounted {
	GDCLASS(GdmlRenderer, RefCounted)

	std::unique_ptr<GdmlParser> m_parser;

protected:
	static void _bind_methods();

public:
	GdmlRenderer();
	Ref<GDMLResult> renderString(const String &source, const godot::Dictionary &data);
	Ref<GDMLResult> renderMarkup(GdmlScript *script, const godot::Dictionary &data);
};

} //namespace godot
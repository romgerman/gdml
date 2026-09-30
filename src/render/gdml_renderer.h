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

namespace traversal {
class TreeRendererVisitor;
}

namespace godot {

class GDMLResult : public RefCounted {
	GDCLASS(GDMLResult, RefCounted)

protected:
	static void _bind_methods();

public:
	godot::Array nodes;
	godot::Dictionary variables;
	godot::Array signals;

	void connect_signal(String signal_name, ObjectID target, StringName target_signal);
	void on_defined_signal(String signal_name);

	godot::Array get_nodes();

	godot::Array get_variable_list();
	godot::Ref<reactive::GDMLAtom> get_variable(godot::String p_name);
	
	godot::Array get_signals();
};

class GDMLRenderer : public RefCounted {
	GDCLASS(GDMLRenderer, RefCounted)

	std::unique_ptr<GdmlParser> m_parser;
	std::unique_ptr<traversal::TreeRendererVisitor> m_visitor;

protected:
	static void _bind_methods();

public:
	GDMLRenderer();
	Ref<GDMLResult> renderString(const String &source);
	Ref<GDMLResult> renderMarkup(GdmlScript *script);
};

} //namespace godot
#pragma once

#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"
#include "gdml_atom.h"
#include "renderer_objects.hpp"

#include <memory>
#include <functional>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace traversal {
class TreeRendererVisitor;
class FragmentContext;
}

namespace gdml {

class GDMLResult : public godot::RefCounted {
	friend class GDMLRenderer;
	GDCLASS(GDMLResult, godot::RefCounted)

	struct ConnectedSignal {
		godot::Callable callable;
		uint32_t flags;
	};

protected:
	godot::Dictionary m_variables;
	godot::Node *m_root;
	godot::Array m_nodes;
	bool is_first_renderer = true;
	
	static void _bind_methods();
	
public:
	GDMLResult();
	
	void add_variable(godot::String &p_name, godot::Ref<reactive::GDMLAtom> atom);
	void attach(bool p_set_owner = true);
	godot::Node *get_root();

	std::vector<std::shared_ptr<traversal::FragmentContext>> m_owned_fragments;
	std::map<godot::String, gdml::SignalTarget> m_signals;
	std::map<godot::String, ConnectedSignal> m_connected_signals;
	
	// API

	godot::Array get_variable_list();
	godot::Ref<reactive::GDMLAtom> get_variable(godot::String p_name);
	
	godot::Array get_signals();
	godot::Error connect_signal(godot::String p_signal_name, godot::Callable p_callable, uint32_t p_flags = 0U);
};

class GDMLRenderer : public godot::RefCounted {
	GDCLASS(GDMLRenderer, godot::RefCounted)

	std::unique_ptr<godot::GdmlParser> m_parser;
	std::unique_ptr<traversal::TreeRendererVisitor> m_visitor;

protected:
	static void _bind_methods();

public:
	GDMLRenderer();
	~GDMLRenderer();
	godot::Ref<GDMLResult> render_string(godot::Node *p_root, const godot::String &p_source, godot::Array variables = godot::Array());
	godot::Ref<GDMLResult> render_markup(godot::Node *p_root, godot::GdmlScript *p_script, godot::Array variables = godot::Array());
};

} //namespace godot

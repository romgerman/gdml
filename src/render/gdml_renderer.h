#pragma once

#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"
#include "gdml_atom.h"

#include <map>
#include <memory>
#include <functional>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace traversal {
class TreeRendererVisitor;
}

namespace gdml {

class GDMLResult : public godot::RefCounted {
	friend class GDMLRenderer;
	GDCLASS(GDMLResult, godot::RefCounted)

	template<typename... Args>
	void on_defined_signal(Args ...args, godot::String signal_name, godot::Variant target);
	void on_defined_signal0(godot::String signal_name, godot::Variant target);
	void on_defined_signal1(godot::Variant arg1, godot::String signal_name, godot::Variant target);
	void on_defined_signal2(godot::Variant arg1, godot::Variant arg2, godot::String signal_name, godot::Variant target);
	void on_defined_signal3(godot::Variant arg1, godot::Variant arg2, godot::Variant arg3, godot::String signal_name, godot::Variant target);

protected:
	godot::Dictionary m_variables;
	godot::Array m_signals;
	godot::Node *m_root;
	godot::Array m_nodes;
	bool is_first_renderer = true;
	
	static void _bind_methods();
	
public:
	GDMLResult();
	
	void add_variable(godot::String &p_name, godot::Ref<reactive::GDMLAtom> atom);
	void connect_signal(godot::String signal_name, godot::ObjectID target, godot::StringName target_signal);
	void attach(bool p_set_owner = true);
	godot::Node *get_root();

	// API

	godot::Array get_variable_list();
	godot::Ref<reactive::GDMLAtom> get_variable(godot::String p_name);
	
	godot::Array get_signals();
};

class GDMLRenderer : public godot::RefCounted {
	GDCLASS(GDMLRenderer, godot::RefCounted)

	std::unique_ptr<godot::GdmlParser> m_parser;
	std::unique_ptr<traversal::TreeRendererVisitor> m_visitor;

protected:
	static void _bind_methods();

public:
	GDMLRenderer();
	godot::Ref<GDMLResult> render_string(godot::Node *p_root, const godot::String &p_source);
	godot::Ref<GDMLResult> render_markup(godot::Node *p_root, godot::GdmlScript *p_script);
};

} //namespace godot
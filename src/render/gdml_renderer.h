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

namespace godot {

class GDMLResult : public RefCounted {
	friend class GDMLRenderer;
	GDCLASS(GDMLResult, RefCounted)

	template<typename... Args>
	void on_defined_signal(Args ...args, String signal_name, Variant target);
	void on_defined_signal0(String signal_name, Variant target);
	void on_defined_signal1(Variant arg1, String signal_name, Variant target);
	void on_defined_signal2(Variant arg1, Variant arg2, String signal_name, Variant target);
	void on_defined_signal3(Variant arg1, Variant arg2, Variant arg3, String signal_name, Variant target);

protected:
	Dictionary m_variables;
	Array m_signals;
	Node *m_root;
	Array m_nodes;
	bool is_first_renderer = true;
	
	static void _bind_methods();
	
public:
	GDMLResult();
	
	void add_variable(String &p_name, Ref<reactive::GDMLAtom> atom);
	void connect_signal(String signal_name, ObjectID target, StringName target_signal);
	void attach(bool p_set_owner = true);
	Node *get_root();

	// API

	Array get_variable_list();
	Ref<reactive::GDMLAtom> get_variable(String p_name);
	
	Array get_signals();
};

class GDMLRenderer : public RefCounted {
	GDCLASS(GDMLRenderer, RefCounted)

	std::unique_ptr<GdmlParser> m_parser;
	std::unique_ptr<traversal::TreeRendererVisitor> m_visitor;

protected:
	static void _bind_methods();

public:
	GDMLRenderer();
	Ref<GDMLResult> renderString(Node *p_root, const String &p_source);
	Ref<GDMLResult> renderMarkup(Node *p_root, GdmlScript *p_script);
};

} //namespace godot
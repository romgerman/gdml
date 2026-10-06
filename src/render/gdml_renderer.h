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

protected:
	Array m_nodes;
	Dictionary m_variables;
	Array m_signals;
	
	static void _bind_methods();
	
public:
	GDMLResult();
	~GDMLResult();
	
	void add_variable(String &p_name, Ref<reactive::GDMLAtom> atom);

	void connect_signal(String signal_name, ObjectID target, StringName target_signal);
	template<typename... Args>
	void on_defined_signal(Args ...args, String signal_name, Variant target);
	void on_defined_signal0(String signal_name, Variant target);
	void on_defined_signal1(Variant arg1, String signal_name, Variant target);
	void on_defined_signal2(Variant arg1, Variant arg2, String signal_name, Variant target);

	// API

	Array get_nodes();

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
	Ref<GDMLResult> renderString(const String &source);
	Ref<GDMLResult> renderMarkup(GdmlScript *script);
};

} //namespace godot
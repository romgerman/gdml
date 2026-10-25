#pragma once

#include "gdml_parser.h"
#include "gdml_atom.h"
#include "tree_renderer_scope.h"

#include <vector>
#include <map>
#include <set>
#include <functional>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>

namespace traversal {

struct TreeRendererVisitorContext {
	struct Component {
		godot::NodeDefinitionNode *node;
		std::vector<godot::String> params;
	};

	typedef std::map<godot::ObjectID, std::set<godot::StringName>> SignalTarget;

	std::vector<std::function<void()>> m_deferred_properties;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> m_variables;
	// signal definition name <-> [signal target <-> [signal names]]
	std::map<godot::String, SignalTarget> m_signals;
	std::map<godot::String, godot::String> m_imports;
	std::map<godot::String, Component> m_components;
	std::vector<std::map<godot::String, bool>> m_active_variables;
	uint32_t m_active_context_index = -1;

	void set_reactive_variable(godot::String &p_name, godot::Variant p_value);
	godot::Ref<reactive::GDMLAtom> get_reactive_variable(godot::String &p_name);
	bool has_reactive_variable(godot::String &p_name);

	bool has_global_signal(godot::String &p_name);

	void push_active_context();
	void pop_active_context();
	void set_active_variable(godot::String &p_name);
	std::vector<godot::Ref<reactive::GDMLAtom>> get_active_variables();
	void reset_active_variable(godot::String &p_name);

	void flush_deferred_properties();

	std::shared_ptr<Scope> m_global_scope;
	std::shared_ptr<Scope> m_current_scope;

	Scope *get_scope();
	void push_scope();
	void pop_scope();

	TreeRendererVisitorContext();
};

}
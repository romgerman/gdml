#pragma once

#include "gdml_parser.h"
#include "gdml_atom.h"
#include "tree_renderer_scope.h"
#include "gdml_renderer.h"
#include "gdml_decorator.h"
#include "renderer_objects.hpp"

#include <vector>
#include <map>
#include <functional>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>

namespace traversal {
class PropertyHelper;
struct TreeRendererVisitorContext;
struct FragmentContext;

class DeferredAction {
public:
	virtual ~DeferredAction() = default;
	virtual void apply() = 0;
};

class DeferredSetPropertyAction: public DeferredAction {
	PropertyHelper *m_prop_helper;
	godot::Variant &m_root_variant;
	godot::Variant &m_variant;
	godot::String &m_prop_key;
	godot::ExpressionNode &m_expr;

public:
	DeferredSetPropertyAction(
		PropertyHelper *p_prop_helper,
		godot::Variant &p_root_variant,
		godot::Variant &p_variant,
		godot::String &p_prop_key,
		godot::ExpressionNode &p_expr
	);
	void apply() override;
};

class DeferredAtomRenderCallbackAction: public DeferredAction {
	TreeRendererVisitorContext *m_context;
	godot::String m_var_name;
	std::weak_ptr<FragmentContext> m_fragment;
	std::function<void ()> m_render_callback;
public:
	DeferredAtomRenderCallbackAction(
		TreeRendererVisitorContext *p_context,
		godot::String &p_var_name,
		std::weak_ptr<FragmentContext> p_fragment,
		std::function<void ()> p_render_callback
	);
	void apply() override;
};

class DeferredGlobalSignalConnect: public DeferredAction {
	TreeRendererVisitorContext *m_context;
	godot::String m_signal_name;
	godot::ObjectID m_target;
	godot::StringName m_target_signal;
	godot::Array m_args;
public:
	DeferredGlobalSignalConnect(
		TreeRendererVisitorContext *p_context,
		godot::String p_signal_name,
		godot::ObjectID p_target,
		godot::StringName p_target_signal,
		godot::Array p_args
	);
	void apply() override;
};

class DeferredSignalUpdate: public DeferredAction {
	TreeRendererVisitorContext *m_context;
	godot::Ref<gdml::GDMLResult> m_result;
public:
	DeferredSignalUpdate(TreeRendererVisitorContext *p_context, godot::Ref<gdml::GDMLResult> p_result);
	void apply() override;
};

class DeferredDecoratorCall: public DeferredAction {
	godot::Array m_args;
	godot::Ref<gdml::GDMLResult> m_result;
public:
	godot::Ref<gdml::GDMLDecorator> m_decorator;
	godot::ObjectID m_node;
	
	DeferredDecoratorCall(
		godot::Array p_args,
		godot::ObjectID p_node,
		godot::Ref<gdml::GDMLDecorator> p_decorator,
		godot::Ref<gdml::GDMLResult> p_result
	);
	void apply() override;
};

// Context

struct TreeRendererVisitorContext {
	struct Component {
		godot::NodeDefinitionNode *node;
		std::vector<godot::String> params;
	};
	
	
	std::vector<std::unique_ptr<DeferredAction>> m_deferred_actions;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> m_variables;
	// signal definition name <-> [signal target <-> [signal names]]
	std::map<godot::String, gdml::SignalTarget> m_signals;
	std::map<godot::String, godot::String> m_imports;
	std::map<godot::String, Component> m_components;

	void set_reactive_variable(godot::String &p_name, godot::Variant p_value);
	godot::Ref<reactive::GDMLAtom> get_reactive_variable(godot::String &p_name);
	bool has_reactive_variable(godot::String &p_name);

	bool has_global_signal(godot::String &p_name);

	void add_deferred_action(std::unique_ptr<DeferredAction> p_action);
	void flush_deferred_actions();

	std::shared_ptr<Scope> m_global_scope;
	std::shared_ptr<Scope> m_current_scope;

	Scope *get_scope();
	void push_scope();
	void pop_scope();

	void push_error(const godot::String p_error, const godot::GdmlNode &node);

	TreeRendererVisitorContext();
};

}

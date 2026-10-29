#include "tree_renderer_context.h"
#include "property_helper.h"
#include "fragment.hpp"

namespace traversal {

void TreeRendererVisitorContext::set_reactive_variable(godot::String &p_name, godot::Variant p_value) {
	if (m_variables.find(p_name) != m_variables.end()) {
		godot::Ref<reactive::GDMLAtom> atom = m_variables[p_name];
		atom->set_value(p_value);
	} else {
		godot::Ref<reactive::GDMLAtom> atom = godot::Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
		atom->create(p_name, p_value);
		m_variables[p_name] = atom;
	}
}

godot::Ref<reactive::GDMLAtom> TreeRendererVisitorContext::get_reactive_variable(godot::String &p_name) {
	if (m_variables.find(p_name) != m_variables.end()) {
		return m_variables[p_name];
	} else {
		return nullptr;
	}
}

bool TreeRendererVisitorContext::has_reactive_variable(godot::String &p_name) {
	return m_variables.find(p_name) != m_variables.end();
}

bool TreeRendererVisitorContext::has_global_signal(godot::String &p_name) {
	return m_signals.find(p_name) != m_signals.end();
}

void TreeRendererVisitorContext::flush_deferred_actions() {
	for (std::unique_ptr<DeferredAction> &action : m_deferred_actions) {
		action->apply();
	}

	m_deferred_actions.clear();
}

Scope *TreeRendererVisitorContext::get_scope() {
	return m_current_scope.get();
}

void TreeRendererVisitorContext::push_scope() {
	m_current_scope = m_current_scope->create_scope();
}

void TreeRendererVisitorContext::pop_scope() {
	m_current_scope = m_current_scope->get_parent_scope();
	if (!m_current_scope) {
		godot::print_error("Trying to pop a scope beyond the global scope");
	}
}

void TreeRendererVisitorContext::push_error(const godot::String p_error, const godot::GdmlNode &node) {
	godot::print_error(p_error, " at [", node.location.start.line, ":", node.location.start.column, "]");
}

TreeRendererVisitorContext::TreeRendererVisitorContext() {
	m_global_scope = std::make_shared<Scope>();
	m_current_scope = m_global_scope;
}

DeferredSetPropertyAction::DeferredSetPropertyAction(
	PropertyHelper *p_prop_helper,
	godot::Variant &p_root_variant,
	godot::Variant &p_variant,
	godot::String &p_prop_key,
	godot::ExpressionNode &p_expr
): m_prop_helper(p_prop_helper), m_root_variant(p_root_variant), m_variant(p_variant), m_prop_key(p_prop_key), m_expr(p_expr) {
}

void DeferredSetPropertyAction::apply() {
	m_prop_helper->set_property(m_root_variant, m_variant, m_prop_key, m_expr);
}

DeferredAtomRenderCallbackAction::DeferredAtomRenderCallbackAction(
	TreeRendererVisitorContext *p_context,
	godot::String &p_var_name,
	std::weak_ptr<FragmentContext> p_fragment,
	std::function<void()> p_render_callback
): m_context(p_context), m_var_name(p_var_name), m_fragment(std::move(p_fragment)), m_render_callback(std::move(p_render_callback)) {
}

void DeferredAtomRenderCallbackAction::apply() {
	if (m_fragment.lock()) {
		m_context->get_reactive_variable(m_var_name)->add_callback(m_fragment, m_render_callback);
	}
}

DeferredSignalUpdate::DeferredSignalUpdate(
	TreeRendererVisitorContext *p_context,
	godot::Ref<gdml::GDMLResult> p_result
): m_context(p_context), m_result(p_result) {
}

void DeferredSignalUpdate::apply() {
	for (std::pair<godot::String, traversal::TreeRendererVisitorContext::SignalTarget> pair : m_context->m_signals) {
		std::vector<godot::ObjectID> to_remove;

		for (std::pair<godot::ObjectID, std::set<godot::StringName>> target_pair : pair.second) {
			godot::Object *target_obj = godot::ObjectDB::get_instance(target_pair.first);

			if (target_obj) {
				for (godot::StringName target_signal : target_pair.second) {
					m_result->connect_signal(pair.first, target_pair.first, target_signal);
				}
			} else {
				to_remove.push_back(target_pair.first);
			}
		}

		for (godot::ObjectID id : to_remove) {
			m_context->m_signals[pair.first].erase(id);
		}
	}
}

DeferredDecoratorCall::DeferredDecoratorCall(godot::Array p_args, godot::ObjectID p_node, godot::Ref<gdml::GDMLDecorator> p_decorator, godot::Ref<gdml::GDMLResult> p_result):
	m_args(p_args), m_node(p_node), m_decorator(p_decorator), m_result(p_result) {
}

void DeferredDecoratorCall::apply() {
	godot::Node *node = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(m_node));
	if (!node) {
		return;
	}

	godot::Array call_args;
	call_args.push_back(node);
	call_args.push_back(m_args);
	call_args.push_back(m_result);

	if (m_decorator->has_method(godot::StringName("modify"))) {
		m_decorator->callv(godot::StringName("modify"), call_args);
	}
}

} //namespace traversal

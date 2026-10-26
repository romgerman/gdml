#include "tree_renderer_context.h"

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

void TreeRendererVisitorContext::push_active_context() {
	m_active_variables.push_back(std::map<godot::String, bool>());
	m_active_context_index++;
}

void TreeRendererVisitorContext::pop_active_context() {
	m_active_variables.pop_back();
	m_active_context_index--;
}

void TreeRendererVisitorContext::set_active_variable(godot::String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = true;
}

std::vector<godot::Ref<reactive::GDMLAtom>> TreeRendererVisitorContext::get_active_variables() {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return std::vector<godot::Ref<reactive::GDMLAtom>>();
	}
	
	std::vector<godot::Ref<reactive::GDMLAtom>> out;

	for (std::pair<godot::String, bool> pair : m_active_variables[m_active_context_index]) {
		if (pair.second) {
			out.push_back(get_reactive_variable(pair.first));
		}
	}

	return out;
}

void TreeRendererVisitorContext::reset_active_variable(godot::String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = false;
}

void TreeRendererVisitorContext::flush_deferred_properties() {
	for (std::function<void()> &fn : m_deferred_properties) {
		fn();
	}

	m_deferred_properties.clear();
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

}

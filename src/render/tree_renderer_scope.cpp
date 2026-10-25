#include "tree_renderer_scope.h"

#include "tree_renderer_context.h"

namespace traversal {

void Scope::set_variable(godot::String p_name, godot::Variant p_value) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			current_scope->m_local_variables[p_name] = p_value;
			return;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	m_local_variables[p_name] = p_value;
}

bool Scope::has_variable(godot::String p_name) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			return true;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return false;
}

godot::Variant Scope::get_variable(godot::String p_name) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			return current_scope->m_local_variables[p_name];
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return godot::Variant();
}

void Scope::set_parent_node(godot::Node *p_node) {
	m_parent_node = p_node;
}

godot::Node *Scope::get_parent_node() {
	Scope *current_scope = this;

	do {
		if (current_scope->m_parent_node) {
			return current_scope->m_parent_node;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return nullptr;
}

std::shared_ptr<Scope> Scope::get_parent_scope() {
	return m_parent_scope;
}

std::shared_ptr<Scope> Scope::create_scope() {
	std::shared_ptr<Scope> scope = std::make_shared<Scope>();
	scope->m_parent_scope = shared_from_this();
	return scope;
}

ScopeGuard::ScopeGuard(TreeRendererVisitorContext &ctx): m_context(ctx) {
	m_context.push_active_context();
	m_context.push_scope();
}

ScopeGuard::~ScopeGuard() {
	m_context.pop_scope();
	m_context.pop_active_context();
}

} //namespace traversal
#pragma once

#include <map>
#include <set>
#include <memory>
#include <vector>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/node.hpp>

namespace traversal {

class TreeRendererVisitorContext;

class Scope : public std::enable_shared_from_this<Scope> {
	std::map<godot::String, godot::Variant> m_local_variables;
	std::shared_ptr<Scope> m_parent_scope;
	godot::Node* m_parent_node;
	
public:
	void set_variable(godot::String p_name, godot::Variant p_value);
	bool has_variable(godot::String p_name);
	godot::Variant get_variable(godot::String p_name);

	void set_parent_node(godot::Node *p_node);
	godot::Node* get_parent_node();

	std::shared_ptr<Scope> get_parent_scope();
	std::shared_ptr<Scope> create_scope();
};

class ScopeGuard {
	TreeRendererVisitorContext &m_context;

public:
	ScopeGuard(TreeRendererVisitorContext &ctx);
	~ScopeGuard();
};

}
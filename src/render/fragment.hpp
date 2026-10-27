#pragma once

#include "tree_renderer_scope.h"
#include "gdml_parser.h"

#include <vector>
#include <map>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/node.hpp>

namespace traversal {

struct FragmentContext {
	godot::ObjectID parent_id;
	std::shared_ptr<Scope> scope;
	godot::StatementNode *statement;
	std::shared_ptr<std::vector<godot::Node *>> rendered_nodes;
	std::vector<std::shared_ptr<FragmentContext>> children;

	void dispose_children();
};

}

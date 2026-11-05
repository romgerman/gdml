#pragma once

#include "gdml_parser.h"
#include "gdml_script.h"
#include "gdml_traversal.h"
#include "gdml_atom.h"
#include "gdml_document.h"
#include "decorator_cache.h"
#include "tree_renderer_scope.h"
#include "tree_renderer_context.h"
#include "variant_registry.h"
#include "property_helper.h"

#include <map>
#include <set>
#include <memory>
#include <vector>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/node.hpp>

namespace gdml {
	class GDMLResult;
}

namespace traversal {

struct FragmentContext;

struct TreeRendererVisitorResult {
	std::vector<godot::Node *> nodes;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> variables;
	std::map<godot::String, TreeRendererVisitorContext::SignalTarget> signals;
};

class TreeRendererVisitor : public GdmlVisitor<TreeRendererVisitorResult> {
	std::unique_ptr<TreeRendererVisitorContext> m_context;
	godot::Ref<gdml::GDMLResult> m_result;
	godot::Node *m_parent;
	gdml::GDMLDocument m_document;
	gdml::DecoratorCache m_decorator_cache;
	std::unique_ptr<PropertyHelper> m_prop_helper;
	std::vector<std::shared_ptr<FragmentContext>> m_fragments_stack;
	std::vector<godot::Ref<reactive::GDMLAtom>> m_initial_variables;

public:
	TreeRendererVisitor(godot::Ref<gdml::GDMLResult> p_result, godot::Node *p_parent, std::vector<godot::Ref<reactive::GDMLAtom>> variables);
	TreeRendererVisitorResult visit(const godot::GdmlNode &root_node) override;

private:
	void visitSourceFile(const godot::SourceFileNode &source_node, TreeRendererVisitorResult &result);
	godot::Node *visitNodeDefinition(const godot::NodeDefinitionNode &node);
	void visitStatement(godot::StatementNode *statement, std::vector<godot::Node *> &out_nodes);
	void renderForStatement(godot::ForStatementNode *statement, const std::shared_ptr<std::vector<godot::Node *>> &nodes);
	void renderIfStatement(godot::IfStatementNode *statement, const std::shared_ptr<std::vector<godot::Node *>> &nodes);
	void visitSignalDefinition(godot::SignalDefinitionNode *definition);
	void visitImportStatement(godot::ImportStatementNode &statement);

	bool trySetVariantProperty(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &value_node);
	bool connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, godot::String &callable_name);
};

} //namespace traversal

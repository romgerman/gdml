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

namespace gdml {
	class GDMLResult;
}

namespace traversal {

struct FragmentContext {
	godot::ObjectID parent_id;
	std::shared_ptr<Scope> scope;
	godot::StatementNode *statement;
	std::shared_ptr<std::vector<godot::Variant>> rendered_nodes;
};

struct TreeRendererVisitorResult {
	std::vector<godot::Variant> nodes;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> variables;
	std::map<godot::String, TreeRendererVisitorContext::SignalTarget> signals;
};

class TreeRendererVisitor : public GdmlVisitor<TreeRendererVisitorResult> {
	std::unique_ptr<TreeRendererVisitorContext> m_context;
	godot::Ref<gdml::GDMLResult> m_result;
	godot::Node *m_root_node;
	gdml::GDMLDocument m_document;
	gdml::DecoratorCache m_decorator_cache;
	std::unique_ptr<PropertyHelper> m_prop_helper;

public:
	TreeRendererVisitor(godot::Ref<gdml::GDMLResult> result, godot::Node *p_root);
	TreeRendererVisitorResult visit(const godot::GdmlNode &root_node) override;

private:
	void visitSourceFile(const godot::SourceFileNode &source_node, TreeRendererVisitorResult &result);
	godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode &node);
	void visitForStatement(godot::ForStatementNode *statement, std::vector<godot::Variant> &out_nodes);
	void renderForStatement(godot::ForStatementNode *statement, const std::shared_ptr<std::vector<godot::Variant>> &nodes, std::vector<godot::Variant> &out_nodes);
	void visitIfStatement(godot::IfStatementNode *statement, std::vector<godot::Variant> &out_nodes);
	void renderIfStatement(godot::IfStatementNode *statement, const std::shared_ptr<std::vector<godot::Variant>> &nodes, std::vector<godot::Variant> &out_nodes);
	void visitSignalDefinition(godot::SignalDefinitionNode &definition);
	void visitImportStatement(godot::ImportStatementNode &statement);

	bool trySetVariantProperty(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &value_node);
	void setControlThemeOverride(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, const godot::ExpressionNode &value_node);

	bool connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, godot::String &callable_name);

	bool doesExpressionHaveSelf(godot::ExpressionNode &expr_node);

	void subscribeToVariable(godot::String &p_variable, godot::String &p_prop, godot::Variant &variant);
	void updateSignals();
};

} //namespace traversal
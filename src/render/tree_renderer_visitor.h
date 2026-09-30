#pragma once

#include "gdml_parser.h"
#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"

#include <map>
#include <memory>
#include <vector>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace traversal {

struct TreeRendererVisitorContext {
	godot::Dictionary data;
};

struct TreeRendererVisitorResult {
	std::vector<godot::Variant> nodes;
	std::map<godot::String, godot::Variant> variables;
};

class TreeRendererVisitor : public GdmlVisitor<TreeRendererVisitorResult> {
	std::map<godot::String, godot::Variant::Type> m_variant_type_map;
	std::unique_ptr<TreeRendererVisitorContext> m_context;

public:
	TreeRendererVisitor();
	TreeRendererVisitorResult visit(const godot::GdmlNode &root_node) override;

private:
	void visitSourceFile(const godot::SourceFileNode &node, TreeRendererVisitorResult &result);
	godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode &node);
	void visitForStatement(godot::ForStatementNode &statement, std::vector<godot::Variant> &out_nodes);

	bool setVariantProperty(godot::Variant &root_variant, godot::Variant &variant, const godot::String &prop_key, const godot::ExpressionNode &value_node);
	bool setVariantPropertyDeep(godot::Variant &root_variant, godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, const godot::ExpressionNode &value_node);
	bool getPropertyValue(godot::Variant &variant, const godot::String &prop_key, godot::Variant &value);
	bool getDeepPropertyValue(godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, godot::Variant &value);

	bool connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, const godot::String &callable_name);

	bool resolveExpression(godot::Variant &root_variant, godot::ExpressionNode &expr_node, godot::Variant &value);
};

} //namespace traversal
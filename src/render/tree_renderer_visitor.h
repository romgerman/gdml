#pragma once

#include "gdml_parser.h"
#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"
#include "gdml_atom.h"

#include <map>
#include <set>
#include <memory>
#include <vector>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	class GDMLResult;
}

namespace traversal {

struct TreeRendererVisitorContext {
	typedef std::map<godot::ObjectID, std::set<godot::StringName>> SignalTarget;

	std::vector<std::function<void()>> m_deferred_properties;
	std::map<godot::String, godot::Variant> m_data;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> m_variables;
	// signal definition name <-> [signal target <-> [signal names]]
	std::map<godot::String, SignalTarget> m_signals;
	std::map<godot::String, godot::String> m_imports;
	std::vector<std::map<godot::String, bool>> m_active_variables;
	uint32_t m_active_context_index = -1;

	void set_data(const godot::String &p_name, godot::Variant p_value);
	bool has_data(godot::String &p_name);
	godot::Variant get_data(godot::String &p_name);

	void set_variable(godot::String &p_name, godot::Variant p_value);
	godot::Ref<reactive::GDMLAtom> get_variable(godot::String &p_name);
	bool has_variable(godot::String &p_name);

	void push_active_context();
	void pop_active_context();
	void set_active_variable(godot::String &p_name);
	std::vector<godot::Ref<reactive::GDMLAtom>> get_active_variables();
	void reset_active_variable(godot::String &p_name);

	void flush_deferred_properties();
};

struct TreeRendererVisitorResult {
	std::vector<godot::Variant> nodes;
	std::map<godot::String, godot::Ref<reactive::GDMLAtom>> variables;
	std::map<godot::String, TreeRendererVisitorContext::SignalTarget> signals;
};

class TreeRendererVisitor : public GdmlVisitor<TreeRendererVisitorResult> {
	std::map<godot::String, godot::Variant::Type> m_variant_type_map;
	std::unique_ptr<TreeRendererVisitorContext> m_context;
	godot::Ref<godot::GDMLResult> m_result;

public:

	TreeRendererVisitor(godot::Ref<godot::GDMLResult> result);
	TreeRendererVisitorResult visit(const godot::GdmlNode &root_node) override;

private:
	void visitSourceFile(const godot::SourceFileNode &source_node, TreeRendererVisitorResult &result);
	godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode &node);
	void visitForStatement(godot::ForStatementNode &statement, std::vector<godot::Variant> &out_nodes);
	void renderForStatement(godot::ForStatementNode &statement, const std::shared_ptr<std::vector<godot::Variant>> &nodes, std::vector<godot::Variant> &out_nodes);
	void visitIfStatement(godot::IfStatementNode &statement, std::vector<godot::Variant> &out_nodes);
	void renderIfStatement(godot::IfStatementNode &statement, const std::shared_ptr<std::vector<godot::Variant>> &nodes, std::vector<godot::Variant> &out_nodes);
	void visitSignalDefinition(godot::SignalDefinitionNode &definition);
	void visitImportStatement(godot::ImportStatementNode &statement);

	bool trySetVariantProperty(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &value_node);
	bool setVariantProperty(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, godot::ExpressionNode &value_node);
	bool setVariantPropertyDeep(godot::Variant &root_variant, godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, godot::ExpressionNode &value_node);
	bool getPropertyValue(godot::Variant &variant, godot::String &prop_key, godot::Variant &value);
	bool getDeepPropertyValue(godot::Variant &root_variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, godot::Variant &value);

	void setControlThemeOverride(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, const godot::ExpressionNode &value_node);

	bool connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, const godot::String &callable_name);

	bool resolveExpression(godot::Variant &root_variant, godot::ExpressionNode &expr_node, godot::Variant &value);
	bool doesExpressionHaveSelf(godot::ExpressionNode &expr_node);

	void subscribeToVariable(godot::String &p_variable, godot::String &p_prop, godot::Variant &variant);
	void updateSignals();

	void printError(const godot::String p_error, const godot::GdmlNode &node);
};

} //namespace traversal
#pragma once

#include "gdml_parser.h"
#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"

#include <map>
#include <memory>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace traversal {

struct TreeRendererVisitorContext {
	godot::Dictionary data;
};

class TreeRendererVisitor : public GdmlVisitor<godot::Variant> {
	std::map<godot::String, godot::Variant::Type> m_variant_type_map;
	std::unique_ptr<TreeRendererVisitorContext> m_context;

public:
	TreeRendererVisitor();
	godot::Variant visit(const godot::GdmlNode &root_node) override;
	void setRendererData(const godot::Dictionary &data);

private:
	godot::Variant visitSourceFile(const godot::SourceFileNode &node);
	godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode &node);

	bool setVariantProperty(godot::Variant &root_variant, godot::Variant &variant, const godot::String &prop_key, const godot::ExpressionNode &value_node);
	bool setVariantPropertyDeep(godot::Variant &root_variant, godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, const godot::ExpressionNode &value_node);
	bool getPropertyValue(godot::Variant &variant, const godot::String &prop_key, godot::Variant &value);
	bool getDeepPropertyValue(godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, godot::Variant &value);

	bool connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, const godot::String &callable_name);

	bool resolveExpression(godot::Variant &root_variant, const godot::ExpressionNode &expr_node, godot::Variant &value);
};

} //namespace traversal

namespace godot {

class GdmlRenderer : public RefCounted {
	GDCLASS(GdmlRenderer, RefCounted)

	std::unique_ptr<GdmlParser> m_parser;

protected:
	static void _bind_methods();

public:
	GdmlRenderer();
	Variant renderString(const String &source, const godot::Dictionary &data);
	Variant renderScript(GdmlScript *script, const godot::Dictionary &data);
};

} //namespace godot
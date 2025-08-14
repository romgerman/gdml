#pragma once

#include "gdml_parser.h"
#include "gdml_traversal.h"
#include "gdml_script_language.h"
#include "gdml_script.h"

#include <memory>
#include <map>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

namespace traversal {

class TreeRendererVisitor : public GdmlVisitor<godot::Variant> {
	std::map<godot::String, godot::Variant::Type> m_variant_type_map;

public:
    godot::Variant visit(const godot::GdmlNode& root_node) override;
private:
    godot::Variant visitSourceFile(const godot::SourceFileNode& node);
    godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode& node);

    bool setVariantProperty(godot::Variant &obj, const godot::String& prop_key, const godot::ExpressionNode& value_node);
	bool setVariantPropertyDeep(godot::Variant &obj, const std::vector<std::unique_ptr<godot::IdentifierNode>>& path, const godot::ExpressionNode& value_node);
};

}

namespace godot {

class GdmlRenderer : public RefCounted {
    GDCLASS(GdmlRenderer, RefCounted)

    std::unique_ptr<GdmlParser> m_parser;

protected:
    static void _bind_methods();

public:
    GdmlRenderer();
    Variant renderString(const String& source, const godot::Dictionary& data);
    Variant renderScript(GdmlScript* script, const godot::Dictionary& data);
};

}
#pragma once

#include "gdml_parser.h"
#include "gdml_traversal.h"
#include "gdml_script_language.h"
#include "gdml_script.h"

#include <memory>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

namespace traversal {

class TreeRendererVisitor : public GdmlVisitor<godot::Variant> {
public:
    godot::Variant visit(const godot::GdmlNode& root_node) override;
private:
    godot::Variant visitSourceFile(const godot::SourceFileNode& node);
    godot::Variant visitNodeDefinition(const godot::NodeDefinitionNode& node);

    void setVariantProperty(godot::Variant &obj, const godot::String& prop_key, const godot::ExpressionNode& value_node);
};

}

namespace godot {

class GdmlRenderer : public RefCounted {
    GDCLASS(GdmlRenderer, RefCounted)

    std::unique_ptr<GdmlParser> gdml;

protected:
    static void _bind_methods();

public:
    GdmlRenderer();
    Variant renderString(const String& source);
    Variant renderScript(GdmlScript* script);
};

}
#pragma once

#include "gdml_parser.h"

#include <memory>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>

class TSNode;

namespace traversal {

template<typename T>
class GdmlVisitor {
public:
    virtual T visit(godot::GdmlNode& root_node) = 0;
};

class GdmlPrinterVisitor : public GdmlVisitor<godot::String> {
public:
    godot::String visit(godot::GdmlNode& root_node);
private:
    godot::String visitNode(godot::GdmlNode& node, uint32_t depth);
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
    TypedArray<Control>* renderString(const String& string);
    PackedScene* preRenderString(const String& string);
private:
    TypedArray<Control>* visitSourceFile(TSNode root_node);
    Control* visitDefinition(TSNode node);
    Control* visitNodeDefinition(TSNode node);
    void visitPropertyDefinition(TSNode node);
    void visitResourceDefinition(TSNode node);
};

}
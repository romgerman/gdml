#pragma once

#include "gdml.h"

#include <memory>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>

class TSNode;

namespace godot {

class GdmlRenderer : public RefCounted {
    GDCLASS(GdmlRenderer, RefCounted)

    std::unique_ptr<Gdml> gdml;

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
};

}
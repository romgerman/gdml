#pragma once

#include "gdml_parser.h"

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

    std::unique_ptr<GdmlParser> gdml;

protected:
    static void _bind_methods();

public:
    GdmlRenderer();
    TypedArray<Control>* renderString(const String& string);
    PackedScene* preRenderString(const String& string);
};

}
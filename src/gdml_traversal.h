#pragma once

#include "gdml_parser.h"

#include <godot_cpp/variant/string.hpp>

namespace traversal {

template<typename T>
class GdmlVisitor {
public:
    virtual T visit(const godot::GdmlNode& root_node) = 0;
};

class GdmlPrinterVisitor : public GdmlVisitor<godot::String> {
public:
    godot::String visit(const godot::GdmlNode& root_node);
private:
    godot::String visitNode(const godot::GdmlNode& node, uint32_t depth);
};

}
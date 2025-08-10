#include "gdml_renderer.h"

#include "gdml_traversal.h"

#include <tree_sitter/api.h>

#include <godot_cpp/core/print_string.hpp>

using namespace godot;

void GdmlRenderer::_bind_methods() {}

GdmlRenderer::GdmlRenderer() {
    gdml = std::make_unique<GdmlParser>();
}

TypedArray<Control>* GdmlRenderer::renderString(const String &string) {
    std::unique_ptr<godot::SourceFileNode> result = gdml->parse(string);
    String treeString = traversal::GdmlPrinterVisitor().visit(*result.get());
    print_line(treeString);
    for (GdmlParseError& err : gdml->getParseErrors()) {
        print_line(err.toString());
    }
    auto tree = gdml->rawParse(string);
    TSNode root = ts_tree_root_node(tree.get());
    return memnew(TypedArray<Control>);
}

PackedScene* GdmlRenderer::preRenderString(const String &string) {
    return memnew(PackedScene);
}

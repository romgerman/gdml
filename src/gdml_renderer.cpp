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
    return visitSourceFile(root);
}

PackedScene* GdmlRenderer::preRenderString(const String &string) {
    return memnew(PackedScene);
}

TypedArray<Control>* GdmlRenderer::visitSourceFile(TSNode root_node) {
    TypedArray<Control>* result = memnew(TypedArray<Control>);
    uint32_t child_count = ts_node_named_child_count(root_node);

    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child_node = ts_node_named_child(root_node, i);
        const char* node_type = ts_node_type(child_node);

        if (strcmp(node_type, "definition") == 0) {
            visitDefinition(child_node);
        }
    }

    return result;
}

Control* GdmlRenderer::visitDefinition(TSNode node) {
    print_line("inside definitoin");
    return memnew(Control);
}

Control* GdmlRenderer::visitNodeDefinition(TSNode node) {
    return memnew(Control);
}

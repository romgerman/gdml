#include "gdml_renderer.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

// Visitor

Variant traversal::TreeRendererVisitor::visit(const godot::GdmlNode& root_node) {
    if (root_node.type == GdmlNodeType::SOURCE_FILE) {
        return visitSourceFile((godot::SourceFileNode&)root_node);
    } else {
        return nullptr;
    }
}

godot::Variant traversal::TreeRendererVisitor::visitSourceFile(const godot::SourceFileNode& node) {
    if (node.statements.size() > 1) {
        return nullptr;
    }

    for (const std::unique_ptr<godot::DefinitionNode>& statement : node.statements) {
        if (!statement->node) {
            return nullptr;
        }

        return visitNodeDefinition(*statement->node.get());
    }

    return nullptr;
}

godot::Variant traversal::TreeRendererVisitor::visitNodeDefinition(const godot::NodeDefinitionNode &node) {
    if (ClassDB::class_exists(node.name->name)) {
        godot::Variant control = ClassDB::instantiate(node.name->name);

        return control;
    }

    return nullptr;
}

// Renderer

void GdmlRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("renderString"), &GdmlRenderer::renderString);
    ClassDB::bind_method(D_METHOD("renderScript"), &GdmlRenderer::renderScript);
}

GdmlRenderer::GdmlRenderer() : gdml(std::make_unique<GdmlParser>()) {}

Variant GdmlRenderer::renderString(const String &source) {
    std::unique_ptr<godot::SourceFileNode> result = gdml->parse(source);
    String treeString = traversal::GdmlPrinterVisitor().visit(*result.get());
    print_line(treeString);
    for (GdmlParseError& err : gdml->getParseErrors()) {
        print_line(err.toString());
    }
    return traversal::TreeRendererVisitor().visit(*result.get());
}

Variant godot::GdmlRenderer::renderScript(GdmlScript* script) {
  return renderString(script->get_source_code());
}

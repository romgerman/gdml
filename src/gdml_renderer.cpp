#include "gdml_renderer.h"

#include <tree_sitter/api.h>

#include <godot_cpp/core/print_string.hpp>

using namespace godot;

String traversal::GdmlPrinterVisitor::visit(GdmlNode& root_node) {
    return visitNode(root_node, 0);
}

String traversal::GdmlPrinterVisitor::visitNode(GdmlNode& node, uint32_t depth) {
    if (node.type == GdmlNodeType::SOURCE_FILE) {
        SourceFileNode& self = (SourceFileNode&)node;
        String out;
        for (const std::unique_ptr<DefinitionNode>& statement : self.statements) {
            out += visitNode(*statement.get(), depth + 1);
        }
        return out;
    } else if (node.type == GdmlNodeType::DEFINITION) {
        DefinitionNode& self = (DefinitionNode&)node;
        return visitNode(*self.node.get(), depth);
    } else if (node.type == GdmlNodeType::NODE_DEFINITION) {
        NodeDefinitionNode& self = (NodeDefinitionNode&)node;
        String out = self.name->name + " {\n";
        for (const std::unique_ptr<PropertyDefinitionNode>& prop : self.properties) {
            out += visitNode(*prop.get(), depth + 1) + "\n";
        }
        return out + "}";
    } else if (node.type == GdmlNodeType::PROPERTY_DEFINITION) {
        PropertyDefinitionNode& self = (PropertyDefinitionNode&)node;
        String out = visitNode(*self.name.get(), depth) + " : " + visitNode(*self.value.get(), depth);
        return out.lpad(out.length() + depth);
    } else if (node.type == GdmlNodeType::RESOURCE_DEFINITION) {
        ResourceDefinitionNode& self = (ResourceDefinitionNode&)node;
        String out = self.name->name + " {\n";
        for (const std::unique_ptr<PropertyDefinitionNode>& prop : self.properties) {
            out += visitNode(*prop.get(), depth + 1) + "\n";
        }
        return out + "}";
    } else if (node.type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
        IdentifierNode& self = (IdentifierNode&)node;
        return self.name;
    } else if (node.type == GdmlNodeType::STRING_EXPRESSION) {
        StringNode& self = (StringNode&)node;
        return "\"" + self.value + "\"";
    } else if (node.type == GdmlNodeType::NUMBER_EXPRESSION) {
        NumberNode& self = (NumberNode&)node;
        return self.value;
    } else if (node.type == GdmlNodeType::BOOLEAN_EXPRESSION) {
        BooleanNode& self = (BooleanNode&)node;
        return String(self.value ? "true" : "false");
    } else {
        return String("(node not implemented)");
    }
}

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

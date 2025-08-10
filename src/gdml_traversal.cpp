#include "gdml_traversal.h"

using namespace godot;

String traversal::GdmlPrinterVisitor::visit(const GdmlNode& root_node) {
    return visitNode(root_node, 0);
}

String traversal::GdmlPrinterVisitor::visitNode(const GdmlNode& node, uint32_t depth) {
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

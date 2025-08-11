#include "gdml_traversal.h"

#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

String traversal::GdmlPrinterVisitor::visit(const GdmlNode &root_node) {
	return visitNode(root_node, 0);
}

String traversal::GdmlPrinterVisitor::visitNode(const GdmlNode &node, uint32_t depth) {
	switch (node.type) {
		case GdmlNodeType::SOURCE_FILE: {
			SourceFileNode &self = (SourceFileNode &)node;
			String out;
			for (const std::unique_ptr<DefinitionNode> &statement : self.statements) {
				out += visitNode(*statement.get(), depth + 1);
			}
			return out;
		} break;
		case GdmlNodeType::DEFINITION: {
			DefinitionNode &self = (DefinitionNode &)node;
			return visitNode(*self.node.get(), depth);
		} break;
		case GdmlNodeType::NODE_DEFINITION: {
			NodeDefinitionNode &self = (NodeDefinitionNode &)node;
			String out = self.name->name + " {\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.properties) {
				out += visitNode(*prop.get(), depth + 1) + "\n";
			}
			for (const std::unique_ptr<NodeDefinitionNode> &child : self.children) {
				out += visitNode(*child.get(), depth + 1) + "\n";
			}
			return out + "}";
		} break;
		case GdmlNodeType::PROPERTY_DEFINITION: {
			PropertyDefinitionNode &self = (PropertyDefinitionNode &)node;
			String out = visitNode(*self.name.get(), depth) + " : " + visitNode(*self.value.get(), depth);
			return out.lpad(out.length() + depth);
		} break;
		case GdmlNodeType::RESOURCE_DEFINITION: {
			ResourceDefinitionNode &self = (ResourceDefinitionNode &)node;
			String out = self.name->name + " {\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.properties) {
				out += visitNode(*prop.get(), depth + 1) + "\n";
			}
			return out + "}";
		} break;
		case GdmlNodeType::IDENTIFIER_EXPRESSION: {
			IdentifierNode &self = (IdentifierNode &)node;
			return self.name;
		} break;
		case GdmlNodeType::MEMBER_ACCESS_EXPRESSION: {
			MemberExpressionNode &self = (MemberExpressionNode &)node;
			godot::PackedStringArray arr;
			for (const std::unique_ptr<godot::IdentifierNode> &part : self.path) {
				arr.append(part->name);
			}
			return String(".").join(arr);
		} break;
		case GdmlNodeType::STRING_EXPRESSION: {
			StringNode &self = (StringNode &)node;
			return "\"" + self.value + "\"";
		} break;
		case GdmlNodeType::NUMBER_EXPRESSION: {
			NumberNode &self = (NumberNode &)node;
			return self.value;
		} break;
		case GdmlNodeType::BOOLEAN_EXPRESSION: {
			BooleanNode &self = (BooleanNode &)node;
			return String(self.value ? "true" : "false");
		} break;
		case GdmlNodeType::ARRAY_EXPRESSION: {
			ArrayExpressionNode &self = (ArrayExpressionNode &)node;
			String out = "[";
			for (const std::unique_ptr<godot::ExpressionNode> &expr : self.values) {
				out += visitNode(*expr.get(), depth) + ", ";
			}
			return out + "]";
		} break;
		case GdmlNodeType::DICTIONARY_EXPRESSION: {
			DictionaryExpressionNode &self = (DictionaryExpressionNode &)node;
			String out = "{\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.values) {
				out += visitNode(*prop.get(), depth + 1) + "\n";
			}
			return out + "}";
		} break;
		default:
			return String("(node not implemented)");
	}
}

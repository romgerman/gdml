#include "gdml_traversal.h"

#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;
using namespace traversal;

String GdmlPrinterVisitor::visit(const GdmlNode &root_node) {
	return visitNode(root_node, 0);
}

String GdmlPrinterVisitor::visitNode(const GdmlNode &node, uint32_t depth) {
	switch (node.type) {
		case GdmlNodeType::SOURCE_FILE: {
			const SourceFileNode &self = static_cast<const SourceFileNode &>(node);
			String out;
			for (const std::unique_ptr<DefinitionNode> &statement : self.statements) {
				out += visitNode(*statement, depth + 1);
			}
			return out;
		} break;
		case GdmlNodeType::DEFINITION: {
			const DefinitionNode &self = static_cast<const DefinitionNode &>(node);
			return visitNode(*self.node, depth);
		} break;
		case GdmlNodeType::NODE_DEFINITION: {
			const NodeDefinitionNode &self = static_cast<const NodeDefinitionNode &>(node);
			String out = self.name->name + " {\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.properties) {
				out += visitNode(*prop, depth + 1) + "\n";
			}
			for (const std::unique_ptr<NodeDefinitionNode> &child : self.children) {
				out += visitNode(*child, depth + 1) + "\n";
			}
			return out + "}";
		} break;
		case GdmlNodeType::PROPERTY_DEFINITION: {
			const PropertyDefinitionNode &self = static_cast<const PropertyDefinitionNode &>(node);
			String out = visitNode(*self.name, depth) + " : " + visitNode(*self.value, depth);
			return depthPad(out, depth);
		} break;
		case GdmlNodeType::RESOURCE_DEFINITION: {
			const ResourceDefinitionNode &self = static_cast<const ResourceDefinitionNode &>(node);
			String out = self.name->name + " {\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.properties) {
				out += visitNode(*prop, depth + 1) + "\n";
			}
			return out + "}";
		} break;
		case GdmlNodeType::SIGNAL_DEFINITION: {
			const SignalDefinitionNode &self = static_cast<const SignalDefinitionNode &>(node);
			String out = "signal " + self.name->name + "\n";
			return out;
		} break;
		case GdmlNodeType::VARIABLE_DEFINITION: {
			const VariableDefinitionNode &self = static_cast<const VariableDefinitionNode &>(node);
			String out = "let " + self.name->name + " = " + visitNode(*self.value, depth) + "\n";
			return out;
		} break;
		case GdmlNodeType::IDENTIFIER_EXPRESSION: {
			const IdentifierNode &self = static_cast<const IdentifierNode &>(node);
			return self.name;
		} break;
		case GdmlNodeType::MEMBER_ACCESS_EXPRESSION: {
			const MemberExpressionNode &self = static_cast<const MemberExpressionNode &>(node);
			godot::PackedStringArray arr;
			for (const std::unique_ptr<IdentifierNode> &part : self.path) {
				arr.append(part->name);
			}
			return String(".").join(arr);
		} break;
		case GdmlNodeType::STRING_EXPRESSION: {
			const StringNode &self = static_cast<const StringNode &>(node);
			return "\"" + self.value.c_escape() + "\"";
		} break;
		case GdmlNodeType::NUMBER_EXPRESSION: {
			const NumberNode &self = static_cast<const NumberNode &>(node);
			return self.value;
		} break;
		case GdmlNodeType::BOOLEAN_EXPRESSION: {
			const BooleanNode &self = static_cast<const BooleanNode &>(node);
			return String(self.value ? "true" : "false");
		} break;
		case GdmlNodeType::ARRAY_EXPRESSION: {
			const ArrayExpressionNode &self = static_cast<const ArrayExpressionNode &>(node);
			String out = "[";
			for (const std::unique_ptr<ExpressionNode> &expr : self.values) {
				out += visitNode(*expr, depth) + ", ";
			}
			return out + "]";
		} break;
		case GdmlNodeType::DICTIONARY_EXPRESSION: {
			const DictionaryExpressionNode &self = static_cast<const DictionaryExpressionNode &>(node);
			String out = "{\n";
			for (const std::unique_ptr<PropertyDefinitionNode> &prop : self.values) {
				out += visitNode(*prop, depth + 1) + "\n";
			}
			return out + "}";
		} break;
		case GdmlNodeType::SIGNAL_EXPRESSION: {
			const SignalExpressionNode& self = static_cast<const SignalExpressionNode&>(node);
			return "@" + visitNode(*self.name, depth);
		} break;
		case GdmlNodeType::FUNCTION_CALL: {
			const FunctionCallExpressionNode& self = static_cast<const FunctionCallExpressionNode &>(node);
			String out = visitNode(*self.name, 0) + "(";
			for (const std::unique_ptr<ExpressionNode> &expr: self.arguments) {
				out += visitNode(*expr, 0) + ", ";
			}
			return out + ")";
		} break;
		case GdmlNodeType::UNKNOWN: {
			return String("(Unknown node)");
		} break;
		default:
			return String("(node not implemented)");
	}
}

godot::String traversal::GdmlPrinterVisitor::depthPad(const godot::String &text, uint32_t depth) {
	return text.lpad(text.length() + depth);
}

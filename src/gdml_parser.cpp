#include "gdml_parser.h"

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-gdml.h>

#include <godot_cpp/core/print_string.hpp>

using namespace godot;

namespace parser {

GdmlSourceLocation::GdmlSourceLocation(ts::Node node) {
	span = GdmlTextSpan(node.getStartByte(), node.getEndByte());
	TSPoint start_point = node.getStartPoint();
	start = GdmlLinePosition(start_point.row + 1, start_point.column + 1);
	TSPoint end_point = node.getEndPoint();
	end = GdmlLinePosition(end_point.row + 1, end_point.column + 1);
}

String GdmlSourceLocation::getText(const String &text) {
	return text.substr(span.start, span.getLength());
}

godot::String GdmlParseError::toString() {
	return message + " at " + godot::String::num_uint64(location.start.line) + ":" + godot::String::num_uint64(location.start.column);
}

uint32_t GdmlTextSpan::getLength() {
	return end - start;
}

bool GdmlTextSpan::isEmpty() {
	return getLength() == 0;
}

} //namespace parser

GdmlParser::GdmlParser() :
		m_parser(ts_parser_new(), ts_parser_delete) {
	ts_parser_set_language(m_parser.get(), tree_sitter_gdml());
}

std::unique_ptr<TSTree, decltype(&ts_tree_delete)> GdmlParser::rawParse(const String &text) {
	return std::unique_ptr<TSTree, decltype(&ts_tree_delete)>(
			ts_parser_parse_string(m_parser.get(), nullptr, text.utf8().get_data(), text.utf8().length()),
			ts_tree_delete);
}

std::unique_ptr<SourceFileNode> GdmlParser::parse(const String &text) {
	parse_errors.clear();
	auto tree = rawParse(text);
	ts::Node root_node = ts::Node::getRootNode(tree.get());

	std::unique_ptr<SourceFileNode> out = std::make_unique<SourceFileNode>();
	out->type = GdmlNodeType::SOURCE_FILE;

	for (uint32_t i = 0; i < root_node.getNamedChildCount(); i++) {
		ts::Node child_node = root_node.getNamedChild(i);

		if (child_node.isType("definition")) {
			std::unique_ptr<DefinitionNode> def = visitDefinition(child_node, text);
			out->statements.push_back(std::move(def));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing source file", child_node);
		}
	}

	return out;
}

std::unique_ptr<DefinitionNode> GdmlParser::visitDefinition(ts::Node node, const String &text) {
	std::unique_ptr<DefinitionNode> out = std::make_unique<DefinitionNode>();
	out->type = GdmlNodeType::DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("node_definition")) {
			out->node = std::move(visitNodeDefinition(child_node, text));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<NodeDefinitionNode> GdmlParser::visitNodeDefinition(ts::Node node, const String &text) {
	std::unique_ptr<NodeDefinitionNode> out = std::make_unique<NodeDefinitionNode>();
	out->type = GdmlNodeType::NODE_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text);
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("node_definition")) {
			std::unique_ptr<NodeDefinitionNode> def = visitNodeDefinition(child_node, text);
			out->children.push_back(std::move(def));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing node definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<PropertyDefinitionNode> GdmlParser::visitProperty(ts::Node node, const String &text) {
	std::unique_ptr<PropertyDefinitionNode> out = std::make_unique<PropertyDefinitionNode>();
	out->type = GdmlNodeType::PROPERTY_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("member_expression")) {
			out->name = visitMemberExpression(child_node, text);
		} else if (child_node.isType("signal_expression")) {
			out->name = visitSignalExpression(child_node, text);
		} else if (child_node.isType("expression")) {
			out->value = visitExpression(child_node, text);
		} else if (child_node.isType("resource_definition")) {
			out->value = visitResource(child_node, text);
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing property", child_node);
		}
	}

	return out;
}

std::unique_ptr<ResourceDefinitionNode> GdmlParser::visitResource(ts::Node node, const String &text) {
	std::unique_ptr<ResourceDefinitionNode> out = std::make_unique<ResourceDefinitionNode>();
	out->type = GdmlNodeType::RESOURCE_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text);
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing resource definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<ExpressionNode> GdmlParser::visitExpression(ts::Node node, const String &text) {
	std::unique_ptr<ExpressionNode> out = std::make_unique<ExpressionNode>();
	out->type = GdmlNodeType::UNKNOWN;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out = visitIdentifier(child_node, text);
		} else if (child_node.isType("number")) {
			out = visitNumber(child_node, text);
		} else if (child_node.isType("boolean")) {
			out = visitBoolean(child_node, text);
		} else if (child_node.isType("string")) {
			out = visitString(child_node, text);
		} else if (child_node.isType("array_expression")) {
			out = visitArrayExpression(child_node, text);
		} else if (child_node.isType("member_expression")) {
			out = visitMemberExpression(child_node, text);
		} else if (child_node.isType("function_call")) {
			out = visitFunctionCallExpression(child_node, text);
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<MemberExpressionNode> GdmlParser::visitMemberExpression(ts::Node node, const String &text) {
	std::unique_ptr<MemberExpressionNode> out = std::make_unique<MemberExpressionNode>();
	out->type = GdmlNodeType::MEMBER_ACCESS_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->path.push_back(std::move(visitIdentifier(child_node, text)));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing member expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<ArrayExpressionNode> GdmlParser::visitArrayExpression(ts::Node node, const String &text) {
	std::unique_ptr<ArrayExpressionNode> out = std::make_unique<ArrayExpressionNode>();
	out->type = GdmlNodeType::ARRAY_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("expression")) {
			std::unique_ptr<ExpressionNode> expr = visitExpression(child_node, text);
			out->values.push_back(std::move(expr));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing array expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<DictionaryExpressionNode> godot::GdmlParser::visitDictionaryExpression(ts::Node node, const String &text) {
	std::unique_ptr<DictionaryExpressionNode> out = std::make_unique<DictionaryExpressionNode>();
	out->type = GdmlNodeType::DICTIONARY_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text);
			out->values.push_back(std::move(def));
		} else if (child_node.isType("comment")) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing dictionary expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<SignalExpressionNode> GdmlParser::visitSignalExpression(ts::Node node, const String &text) {
	std::unique_ptr<SignalExpressionNode> out = std::make_unique<SignalExpressionNode>();
	out->type = GdmlNodeType::SIGNAL_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing signal expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<FunctionCallExpressionNode> godot::GdmlParser::visitFunctionCallExpression(ts::Node node, const String &text) {
	std::unique_ptr<FunctionCallExpressionNode> out = std::make_unique<FunctionCallExpressionNode>();
	out->type = GdmlNodeType::FUNCTION_CALL;

	ts::Node name_node = node.getChildByFieldName("name");
	if (name_node.isType("identifier")) {
		out->name = visitIdentifier(name_node, text);
	} else if (name_node.isType("member_access")) {
		out->name = visitMemberExpression(name_node, text);
	}

	ts::Node arguments_node = node.getChildByFieldName("arguments");
	if (!arguments_node.isNull()) {
		for (uint32_t i = 0; i < arguments_node.getNamedChildCount(); i++) {
			ts::Node child_node = arguments_node.getNamedChild(i);

			if (child_node.isType("expression")) {
				std::unique_ptr<ExpressionNode> expr = visitExpression(child_node, text);
				out->arguments.push_back(std::move(expr));
			} else {
				pushError("Unknown node " + String(child_node.getType()) + " while parsing function call expression", child_node);
			}
		}
	}

	return out;
}

std::unique_ptr<IdentifierNode> GdmlParser::visitIdentifier(ts::Node node, const String &text) {
	std::unique_ptr<IdentifierNode> out = std::make_unique<IdentifierNode>();
	out->type = GdmlNodeType::IDENTIFIER_EXPRESSION;
	out->name = node.getText(text);

	return out;
}

std::unique_ptr<NumberNode> GdmlParser::visitNumber(ts::Node node, const String &text) {
	std::unique_ptr<NumberNode> out = std::make_unique<NumberNode>();
	out->type = GdmlNodeType::NUMBER_EXPRESSION;
	out->value = node.getText(text);
	out->isFloat = out->value.contains(".");

	return out;
}

std::unique_ptr<BooleanNode> GdmlParser::visitBoolean(ts::Node node, const String &text) {
	std::unique_ptr<BooleanNode> out = std::make_unique<BooleanNode>();
	out->type = GdmlNodeType::BOOLEAN_EXPRESSION;

	const String &rawValue = node.getText(text);
	out->value = rawValue == "true";

	return out;
}

std::unique_ptr<StringNode> GdmlParser::visitString(ts::Node node, const String &text) {
	std::unique_ptr<StringNode> out = std::make_unique<StringNode>();
	out->type = GdmlNodeType::STRING_EXPRESSION;
	out->value = String();

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);
		out->value += child_node.getText(text);
	}

	return out;
}

std::vector<parser::GdmlParseError> &GdmlParser::getParseErrors() {
	return parse_errors;
}

void GdmlParser::pushError(const String &error, ts::Node node) {
	parser::GdmlParseError err;
	err.message = error;
	err.location = parser::GdmlSourceLocation(node);
	parse_errors.push_back(err);
}

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

String GdmlSourceLocation::getText(const String &text) const {
	return text.substr(span.start, span.getLength());
}

String GdmlParseError::toString() {
	return message + " at " + String::num_uint64(location.start.line) + ":" + String::num_uint64(location.start.column);
}

uint32_t GdmlTextSpan::getLength() const {
	return end - start;
}

bool GdmlTextSpan::isEmpty() const {
	return getLength() == 0;
}

} //namespace parser

godot::GdmlParser::GdmlParser() :
		m_parser(ts_parser_new(), ts_parser_delete) {
	if (!ts_parser_set_language(m_parser.get(), tree_sitter_gdml())) {
		print_error("Cannot use the tree-sitter grammar");
	}
}

std::unique_ptr<const TSTree, TSTreeDeleter> godot::GdmlParser::rawParse(const CharString &text) const {
	return std::unique_ptr<const TSTree, TSTreeDeleter>(
			ts_parser_parse_string(m_parser.get(), nullptr, text.get_data(), text.length()),
			TSTreeDeleter());
}

std::unique_ptr<SourceFileNode> godot::GdmlParser::parse(const String &text) {
	parse_errors.clear();
	CharString source = text.utf8();
	auto tree = rawParse(source);
	ts::Node root_node = ts::Node::getRootNode(tree.get());
	collectSyntaxErrors(root_node);

	std::unique_ptr<SourceFileNode> out = std::make_unique<SourceFileNode>();
	out->type = GdmlNodeType::SOURCE_FILE;

	for (uint32_t i = 0; i < root_node.getNamedChildCount(); i++) {
		ts::Node child_node = root_node.getNamedChild(i);

		if (child_node.isType("definition")) {
			std::unique_ptr<DefinitionNode> def = visitDefinition(child_node, source);
			out->statements.push_back(std::move(def));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing source file", child_node);
		}
	}

	return out;
}

void godot::GdmlParser::collectSyntaxErrors(ts::Node node) {
	if (!node.hasError()){
		return;
	}

	if (node.isMissing()) {
		pushError("Missing '" + String(node.getType()) + "'", node);
		return;
	}

	if (node.isError()) {
		pushError("Unexpected syntax", node);
		return;
	}

	for (uint32_t i = 0; i < node.getChildCount(); i++) {
		collectSyntaxErrors(node.getChild(i));
	}
}

std::unique_ptr<DefinitionNode> godot::GdmlParser::visitDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<DefinitionNode> out = std::make_unique<DefinitionNode>();
	out->type = GdmlNodeType::DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("node_definition")) {
			out->statement = std::move(visitNodeDefinition(child_node, text));
		} else if (child_node.isType("signal_definition")) {
			out->statement = std::move(visitSignalDefinition(child_node, text));
		} else if (child_node.isType("variable_definition")) {
			out->statement = std::move(visitVariableDefinition(child_node, text));
		} else if (child_node.isType("for_statement")) {
			out->statement = visitForStatement(child_node, text);
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<NodeDefinitionNode> godot::GdmlParser::visitNodeDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<NodeDefinitionNode> out = std::make_unique<NodeDefinitionNode>();
	out->type = GdmlNodeType::NODE_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition") || child_node.isType("theme_property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text, child_node.isType("theme_property_definition"));
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("node_definition")) {
			std::unique_ptr<NodeDefinitionNode> def = visitNodeDefinition(child_node, text);
			out->statements.push_back(std::move(def));
		} else if (child_node.isType("for_statement")) {
			std::unique_ptr<ForStatementNode> def = visitForStatement(child_node, text);
			out->statements.push_back(std::move(def));
		} else if (child_node.isType("variable_definition")) {
			std::unique_ptr<VariableDefinitionNode> def = visitVariableDefinition(child_node, text);
			out->variables.push_back(std::move(def));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing node definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<PropertyDefinitionNode> godot::GdmlParser::visitProperty(ts::Node node, CharString &text, bool is_theme_property) {
	std::unique_ptr<PropertyDefinitionNode> out = std::make_unique<PropertyDefinitionNode>();
	out->type = GdmlNodeType::PROPERTY_DEFINITION;
	out->is_theme_override = is_theme_property;

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
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing property", child_node);
		}
	}

	return out;
}

std::unique_ptr<ResourceDefinitionNode> godot::GdmlParser::visitResource(ts::Node node, CharString &text) {
	std::unique_ptr<ResourceDefinitionNode> out = std::make_unique<ResourceDefinitionNode>();
	out->type = GdmlNodeType::RESOURCE_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text, false);
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing resource definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<SignalDefinitionNode> godot::GdmlParser::visitSignalDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<SignalDefinitionNode> out = std::make_unique<SignalDefinitionNode>();
	out->type = GdmlNodeType::SIGNAL_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing signal definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<VariableDefinitionNode> godot::GdmlParser::visitVariableDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<VariableDefinitionNode> out = std::make_unique<VariableDefinitionNode>();
	out->type = GdmlNodeType::VARIABLE_DEFINITION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("expression")) {
			out->value = visitExpression(child_node, text);
		} else if (child_node.isType("resource_definition")) {
			out->value = visitResource(child_node, text);
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing variable definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<ForStatementNode> godot::GdmlParser::visitForStatement(ts::Node node, CharString &text) {
	std::unique_ptr<ForStatementNode> out = std::make_unique<ForStatementNode>();
	out->type = GdmlNodeType::FOR_STATEMENT;

	ts::Node iter_node = node.getChildByFieldName("iter_name");
	if (!iter_node.isNull()) {
		if (iter_node.isType("identifier")) {
			out->iter_name = visitIdentifier(iter_node, text);
		}
	}

	ts::Node range_node = node.getChildByFieldName("range");
	if (!range_node.isNull()) {
		if (range_node.isType("identifier")) {
			out->range = visitIdentifier(range_node, text);
		} else if (range_node.isType("number")) {
			out->range = visitNumber(range_node, text);
		} else if (range_node.isType("array_expression")) {
			out->range = visitArrayExpression(range_node, text);
		} else if (range_node.isType("member_expression")) {
			out->range = visitMemberExpression(range_node, text);
		}
	}

	for (uint32_t i = 0; i < node.getChildCount(); i++) {
		ts::Node child_node = node.getChild(i);

		if (child_node.isType("node_definition")) {
			out->statements.push_back(visitNodeDefinition(child_node, text));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		}
	}

	return out;
}

std::unique_ptr<ExpressionNode> godot::GdmlParser::visitExpression(ts::Node node, CharString &text) {
	std::unique_ptr<ExpressionNode> out = std::make_unique<ExpressionNode>();
	out->type = GdmlNodeType::UNKNOWN;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			return visitIdentifier(child_node, text);
		} else if (child_node.isType("number")) {
			return visitNumber(child_node, text);
		} else if (child_node.isType("boolean")) {
			return visitBoolean(child_node, text);
		} else if (child_node.isType("string")) {
			return visitString(child_node, text);
		} else if (child_node.isType("array_expression")) {
			return visitArrayExpression(child_node, text);
		} else if (child_node.isType("dictionary_expression")) {
			return visitDictionaryExpression(child_node, text);
		} else if (child_node.isType("member_expression")) {
			return visitMemberExpression(child_node, text);
		} else if (child_node.isType("function_call")) {
			return visitFunctionCallExpression(child_node, text);
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<MemberExpressionNode> godot::GdmlParser::visitMemberExpression(ts::Node node, CharString &text) {
	std::unique_ptr<MemberExpressionNode> out = std::make_unique<MemberExpressionNode>();
	out->type = GdmlNodeType::MEMBER_ACCESS_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->path.push_back(std::move(visitIdentifier(child_node, text)));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing member expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<ArrayExpressionNode> godot::GdmlParser::visitArrayExpression(ts::Node node, CharString &text) {
	std::unique_ptr<ArrayExpressionNode> out = std::make_unique<ArrayExpressionNode>();
	out->type = GdmlNodeType::ARRAY_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("expression")) {
			out->values.push_back(visitExpression(child_node, text));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing array expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<DictionaryExpressionNode> godot::GdmlParser::visitDictionaryExpression(ts::Node node, CharString &text) {
	std::unique_ptr<DictionaryExpressionNode> out = std::make_unique<DictionaryExpressionNode>();
	out->type = GdmlNodeType::DICTIONARY_EXPRESSION;

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text, false);
			out->values.push_back(std::move(def));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing dictionary expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<SignalExpressionNode> godot::GdmlParser::visitSignalExpression(ts::Node node, CharString &text) {
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

std::unique_ptr<FunctionCallExpressionNode> godot::GdmlParser::visitFunctionCallExpression(ts::Node node, CharString &text) {
	std::unique_ptr<FunctionCallExpressionNode> out = std::make_unique<FunctionCallExpressionNode>();
	out->type = GdmlNodeType::FUNCTION_CALL;

	ts::Node name_node = node.getChildByFieldName("name");
	if (!name_node.isNull()) {
		if (name_node.isType("identifier")) {
			out->name = visitIdentifier(name_node, text);
		} else if (name_node.isType("member_expression")) {
			out->name = visitMemberExpression(name_node, text);
		}
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

std::unique_ptr<IdentifierNode> godot::GdmlParser::visitIdentifier(ts::Node node, CharString &text) {
	std::unique_ptr<IdentifierNode> out = std::make_unique<IdentifierNode>();
	out->type = GdmlNodeType::IDENTIFIER_EXPRESSION;
	out->name = node.getText(text);

	return out;
}

std::unique_ptr<NumberNode> godot::GdmlParser::visitNumber(ts::Node node, CharString &text) {
	std::unique_ptr<NumberNode> out = std::make_unique<NumberNode>();
	out->type = GdmlNodeType::NUMBER_EXPRESSION;
	out->value = node.getText(text);
	out->isFloat = out->value.contains(".");

	return out;
}

std::unique_ptr<BooleanNode> godot::GdmlParser::visitBoolean(ts::Node node, CharString &text) {
	std::unique_ptr<BooleanNode> out = std::make_unique<BooleanNode>();
	out->type = GdmlNodeType::BOOLEAN_EXPRESSION;

	const String &rawValue = node.getText(text);
	out->value = rawValue == "true";

	return out;
}

std::unique_ptr<StringNode> godot::GdmlParser::visitString(ts::Node node, CharString &text) {
	std::unique_ptr<StringNode> out = std::make_unique<StringNode>();
	out->type = GdmlNodeType::STRING_EXPRESSION;
	out->value = String();

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);
		out->value += child_node.getText(text);
	}

	return out;
}

std::vector<parser::GdmlParseError> &godot::GdmlParser::getParseErrors() {
	return parse_errors;
}

void godot::GdmlParser::pushError(const String &error, const ts::Node &node) {
	parser::GdmlParseError err;
	err.message = error;
	err.location = parser::GdmlSourceLocation(node);
	parse_errors.push_back(err);
}

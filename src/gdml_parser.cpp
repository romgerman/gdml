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

namespace godot {

GdmlParser::GdmlParser() :
		m_parser(ts_parser_new(), ts_parser_delete) {
	if (!ts_parser_set_language(m_parser.get(), tree_sitter_gdml())) {
		print_error("Cannot use the tree-sitter grammar");
	}
}

std::unique_ptr<const TSTree, TSTreeDeleter> GdmlParser::rawParse(const CharString &text) const {
	return std::unique_ptr<const TSTree, TSTreeDeleter>(
			ts_parser_parse_string(m_parser.get(), nullptr, text.get_data(), text.length()),
			TSTreeDeleter());
}

std::unique_ptr<SourceFileNode> GdmlParser::parse(const String &text) {
	parse_errors.clear();
	CharString source = text.utf8();
	auto tree = rawParse(source);
	ts::Node root_node = ts::Node::getRootNode(tree.get());
	collectSyntaxErrors(root_node);

	std::unique_ptr<SourceFileNode> out = std::make_unique<SourceFileNode>();
	out->type = GDMLNodeType::SOURCE_FILE;
	out->location = parser::GdmlSourceLocation(root_node);

	for (uint32_t i = 0; i < root_node.getNamedChildCount(); i++) {
		ts::Node child_node = root_node.getNamedChild(i);

		if (child_node.isType("import_statement")) {
			out->imports.push_back(visitImport(child_node, source));
		} else if (child_node.isType("definition")) {
			out->statements.push_back(visitDefinition(child_node, source));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing source file", child_node);
		}
	}

	return out;
}

void GdmlParser::collectSyntaxErrors(ts::Node node) {
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

std::unique_ptr<ImportStatementNode> GdmlParser::visitImport(ts::Node node, CharString &text) {
	std::unique_ptr<ImportStatementNode> out = std::make_unique<ImportStatementNode>();
	out->type = GDMLNodeType::IMPORT;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("string")) {
			out->from = visitString(child_node, text);
		} else if (child_node.isType("from") || child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<DefinitionNode> GdmlParser::visitDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<DefinitionNode> out = std::make_unique<DefinitionNode>();
	out->type = GDMLNodeType::DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("node_definition")) {
			out->statement = visitNodeDefinition(child_node, text);
			break;
		} else if (child_node.isType("signal_definition")) {
			out->statement = visitSignalDefinition(child_node, text);
			break;
		} else if (child_node.isType("variable_definition")) {
			out->statement = visitVariableDefinition(child_node, text);
			break;
		} else if (child_node.isType("component_definition")) {
			out->statement = visitComponentDefinition(child_node, text);
			break;
		} else if (child_node.isType("for_statement")) {
			out->statement = visitForStatement(child_node, text);
			break;
		} else if (child_node.isType("if_statement")) {
			out->statement = visitIfStatement(child_node, text);
			break;
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<NodeDefinitionNode> GdmlParser::visitNodeDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<NodeDefinitionNode> out = std::make_unique<NodeDefinitionNode>();
	out->type = GDMLNodeType::NODE_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition") || child_node.isType("theme_property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text, child_node.isType("theme_property_definition"));
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("node_definition")) {
			out->statements.push_back(visitNodeDefinition(child_node, text));
		} else if (child_node.isType("for_statement")) {
			out->statements.push_back(visitForStatement(child_node, text));
		} else if (child_node.isType("if_statement")) {
			out->statements.push_back(visitIfStatement(child_node, text));
		} else if (child_node.isType("variable_definition")) {
			out->variables.push_back(visitVariableDefinition(child_node, text));
		} else if (child_node.isType("function_call")) {
			out->decorators.push_back(visitFunctionCallExpression(child_node, text));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing node definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<PropertyDefinitionNode> GdmlParser::visitProperty(ts::Node node, CharString &text, bool is_theme_property) {
	std::unique_ptr<PropertyDefinitionNode> out = std::make_unique<PropertyDefinitionNode>();
	out->type = GDMLNodeType::PROPERTY_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);
	out->is_theme_override = is_theme_property;

	ts::Node name_node = node.getChildByFieldName("name");
	if (!name_node.isNull()) {
		if (name_node.isType("identifier")) {
			out->name = visitIdentifier(name_node, text);
		} else if (name_node.isType("signal_expression")) {
			out->name = visitSignalExpression(name_node, text);
		} else if (name_node.isType("property_path")) {
			out->name = visitPropertyPath(name_node, text);
		}
	}

	ts::Node value_node = node.getChildByFieldName("value");
	if (!value_node.isNull()) {
		if (value_node.isType("expression")) {
			out->value = visitExpression(value_node, text);
		} else if (value_node.isType("resource_definition")) {
			out->value = visitResource(value_node, text);
		}
	}

	return out;
}

std::unique_ptr<ResourceDefinitionNode> GdmlParser::visitResource(ts::Node node, CharString &text) {
	std::unique_ptr<ResourceDefinitionNode> out = std::make_unique<ResourceDefinitionNode>();
	out->type = GDMLNodeType::RESOURCE_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node shared_node = node.getChildByFieldName("shared");
	if (!shared_node.isNull()) {
		out->is_shared = true;
	}

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->name = visitIdentifier(child_node, text);
		} else if (child_node.isType("property_definition")) {
			std::unique_ptr<PropertyDefinitionNode> def = visitProperty(child_node, text, false);
			out->properties.push_back(std::move(def));
		} else if (child_node.isType("function_call")) {
			out->decorators.push_back(visitFunctionCallExpression(child_node, text));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing resource definition", child_node);
		}
	}

	return out;
}

std::unique_ptr<SignalDefinitionNode> GdmlParser::visitSignalDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<SignalDefinitionNode> out = std::make_unique<SignalDefinitionNode>();
	out->type = GDMLNodeType::SIGNAL_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

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

std::unique_ptr<VariableDefinitionNode> GdmlParser::visitVariableDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<VariableDefinitionNode> out = std::make_unique<VariableDefinitionNode>();
	out->type = GDMLNodeType::VARIABLE_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

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

std::unique_ptr<ComponentDefinitionNode> GdmlParser::visitComponentDefinition(ts::Node node, CharString &text) {
	std::unique_ptr<ComponentDefinitionNode> out = std::make_unique<ComponentDefinitionNode>();
	out->type = GDMLNodeType::COMPONENT_DEFINITION;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node name_node = node.getChildByFieldName("name");
	if (!name_node.isNull()) {
		out->name = visitIdentifier(name_node, text);
	}

	ts::Node params_node = node.getChildByFieldName("params");
	if (!params_node.isNull()) {
		for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
			ts::Node child_node = node.getNamedChild(i);

			if (child_node.isType("identifier") && child_node != name_node) {
				out->params.push_back(visitIdentifier(child_node, text));
			}
		}
	}

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("node_definition")) {
			out->value = visitNodeDefinition(child_node, text);
			break;
		}
	}

	return out;
}

std::unique_ptr<ForStatementNode> GdmlParser::visitForStatement(ts::Node node, CharString &text) {
	std::unique_ptr<ForStatementNode> out = std::make_unique<ForStatementNode>();
	out->type = GDMLNodeType::FOR_STATEMENT;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node iter_node = node.getChildByFieldName("iter_name");
	if (!iter_node.isNull()) {
		if (iter_node.isType("identifier")) {
			out->iter_name = visitIdentifier(iter_node, text);
		}
	}

	ts::Node iter_node2 = node.getChildByFieldName("iter_name2");
	if (!iter_node2.isNull()) {
		if (iter_node2.isType("identifier")) {
			out->iter_name2 = visitIdentifier(iter_node2, text);
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

	ts::Node statements_node = node.getChildByFieldName("statements");
	if (!statements_node.isNull()) {
		for (uint32_t i = 0; i < statements_node.getNamedChildCount(); i++) {
			ts::Node child_node = statements_node.getNamedChild(i);
	
			if (child_node.isType("node_definition")) {
				out->statements.push_back(visitNodeDefinition(child_node, text));
			} else if (child_node.isType("comment") || child_node.isError()) {
				continue;
			}
		}
	}

	return out;
}

std::unique_ptr<IfStatementNode> GdmlParser::visitIfStatement(ts::Node node, CharString &text) {
	std::unique_ptr<IfStatementNode> out = std::make_unique<IfStatementNode>();
	out->type = GDMLNodeType::IF_STATEMENT;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node expr_node = node.getChildByFieldName("condition");
	if (!expr_node.isNull()) {
		if (expr_node.isType("expression")) {
			out->expression = visitExpression(expr_node, text);
		}
	}

	ts::Node if_node = node.getChildByFieldName("if");
	if (!if_node.isNull()) {
		for (uint32_t i = 0; i < if_node.getNamedChildCount(); i++) {
			ts::Node child_node = if_node.getNamedChild(i);

			if (child_node.isType("node_definition")) {
				out->if_statements.push_back(visitNodeDefinition(child_node, text));
			} else if (child_node.isType("comment") || child_node.isError()) {
				continue;
			}
		}
	}

	ts::Node else_node = node.getChildByFieldName("else");
	if (!else_node.isNull()) {
		for (uint32_t i = 0; i < else_node.getNamedChildCount(); i++) {
			ts::Node child_node = else_node.getNamedChild(i);

			if (child_node.isType("node_definition")) {
				out->else_statements.push_back(visitNodeDefinition(child_node, text));
			} else if (child_node.isType("comment") || child_node.isError()) {
				continue;
			}
		}
	}

	return out;
}

std::unique_ptr<PropertyPathNode> GdmlParser::visitPropertyPath(ts::Node node, CharString &text) {
	std::unique_ptr<PropertyPathNode> out = std::make_unique<PropertyPathNode>();
	out->type = GDMLNodeType::PROPERTY_PATH;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("identifier")) {
			out->path.push_back(visitIdentifier(child_node, text));
		} else if (child_node.isError()) {
			continue;
		}
	}

	return out;
}

std::unique_ptr<ExpressionNode> GdmlParser::visitExpression(ts::Node node, CharString &text) {
	if (node.isType("expression")) {
		for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
			ts::Node child_node = node.getNamedChild(i);
			if (child_node.isType("comment") || child_node.isError()) {
				continue;
			}
			return visitExpression(child_node, text);
		}
	} else if (node.isType("identifier")) {
			return visitIdentifier(node, text);
	} else if (node.isType("self")) {
		return visitIdentifier(node, text);
	} else if (node.isType("number")) {
		return visitNumber(node, text);
	} else if (node.isType("boolean")) {
		return visitBoolean(node, text);
	} else if (node.isType("string")) {
		return visitString(node, text);
	} else if (node.isType("array_expression")) {
		return visitArrayExpression(node, text);
	} else if (node.isType("dictionary_expression")) {
		return visitDictionaryExpression(node, text);
	} else if (node.isType("member_expression")) {
		return visitMemberExpression(node, text);
	} else if (node.isType("function_call")) {
		return visitFunctionCallExpression(node, text);
	} else if (node.isType("binary_expression")) {
		return visitBinaryExpression(node, text);
	} else if (node.isType("subscript_expression")) {
		return visitSubscriptExpression(node, text);
	} else if (node.isType("resource_definition")) {
		return visitResource(node, text);
	} else {
		pushError("Unknown node " + String(node.getType()) + " while parsing expression", node);
	}

	std::unique_ptr<ExpressionNode> out = std::make_unique<ExpressionNode>();
	out->type = GDMLNodeType::UNKNOWN;
	out->location = parser::GdmlSourceLocation(node);
	return out;
}

std::unique_ptr<MemberExpressionNode> GdmlParser::visitMemberExpression(ts::Node node, CharString &text) {
	std::unique_ptr<MemberExpressionNode> out = std::make_unique<MemberExpressionNode>();
	out->type = GDMLNodeType::MEMBER_ACCESS_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("expression")) {
			out->expression = visitExpression(child_node, text);
		} else if (child_node.isType("identifier")) {
			out->identifier = visitIdentifier(child_node, text);
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing member expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<ArrayExpressionNode> GdmlParser::visitArrayExpression(ts::Node node, CharString &text) {
	std::unique_ptr<ArrayExpressionNode> out = std::make_unique<ArrayExpressionNode>();
	out->type = GDMLNodeType::ARRAY_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

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

std::unique_ptr<DictionaryExpressionNode> GdmlParser::visitDictionaryExpression(ts::Node node, CharString &text) {
	std::unique_ptr<DictionaryExpressionNode> out = std::make_unique<DictionaryExpressionNode>();
	out->type = GDMLNodeType::DICTIONARY_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

	for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
		ts::Node child_node = node.getNamedChild(i);

		if (child_node.isType("property_definition")) {
			out->values.push_back(visitProperty(child_node, text, false));
		} else if (child_node.isType("comment") || child_node.isError()) {
			continue;
		} else {
			pushError("Unknown node " + String(child_node.getType()) + " while parsing dictionary expression", child_node);
		}
	}

	return out;
}

std::unique_ptr<SignalExpressionNode> GdmlParser::visitSignalExpression(ts::Node node, CharString &text) {
	std::unique_ptr<SignalExpressionNode> out = std::make_unique<SignalExpressionNode>();
	out->type = GDMLNodeType::SIGNAL_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

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

std::unique_ptr<FunctionCallExpressionNode> GdmlParser::visitFunctionCallExpression(ts::Node node, CharString &text) {
	std::unique_ptr<FunctionCallExpressionNode> out = std::make_unique<FunctionCallExpressionNode>();
	out->type = GDMLNodeType::FUNCTION_CALL;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node name_node = node.getChildByFieldName("name");
	if (!name_node.isNull()) {
		out->name = visitExpression(name_node, text);
	}

	ts::Node arguments_node = node.getChildByFieldName("arguments");
	if (!arguments_node.isNull()) {
		for (uint32_t i = 0; i < arguments_node.getNamedChildCount(); i++) {
			ts::Node child_node = arguments_node.getNamedChild(i);

			if (child_node.isType("expression")) {
				out->arguments.push_back(visitExpression(child_node, text));
			} else if (child_node.isType("comment") || child_node.isError()) {
				continue;
			} else {
				pushError("Unknown node " + String(child_node.getType()) + " while parsing function call expression", child_node);
			}
		}
	}

	return out;
}

std::unique_ptr<BinaryExpressionNode> GdmlParser::visitBinaryExpression(ts::Node node, CharString &text) {
	std::unique_ptr<BinaryExpressionNode> out = std::make_unique<BinaryExpressionNode>();
	out->type = GDMLNodeType::BINARY_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node left_node = node.getChildByFieldName("left");
	if (!left_node.isNull()) {
		out->left = visitExpression(left_node, text);
	}

	ts::Node right_node = node.getChildByFieldName("right");
	if (!right_node.isNull()) {
		out->right = visitExpression(right_node, text);
	}

	ts::Node op_node = node.getChildByFieldName("operator");
	if (!op_node.isNull()) {
		String op_str = op_node.getText(text);

		if (op_str == "&&") {
			out->op = GDMLOperator::AND;
		} else if (op_str == "||") {
			out->op = GDMLOperator::OR;
		} else if (op_str == "+") {
			out->op = GDMLOperator::PLUS;
		} else if (op_str == "-") {
			out->op = GDMLOperator::MINUS;
		} else if (op_str == "*") {
			out->op = GDMLOperator::MUL;
		} else if (op_str == "/") {
			out->op = GDMLOperator::DIV;
		} else if (op_str == "<") {
			out->op = GDMLOperator::LESS_THAN;
		} else if (op_str == "<=") {
			out->op = GDMLOperator::LESS_OR_EQUAL_THAN;
		} else if (op_str == "==") {
			out->op = GDMLOperator::EQUAL;
		} else if (op_str == "!=") {
			out->op = GDMLOperator::NOT_EQUAL;
		} else if (op_str == ">=") {
			out->op = GDMLOperator::GREATER_OR_EQUAL_THAN;
		} else if (op_str == ">") {
			out->op = GDMLOperator::GREATER_THAN;
		}
	}

	return out;
}

std::unique_ptr<SubscriptExpressionNode> GdmlParser::visitSubscriptExpression(ts::Node node, CharString &text) {
	std::unique_ptr<SubscriptExpressionNode> out = std::make_unique<SubscriptExpressionNode>();
	out->type = GDMLNodeType::SUBSCRIPT_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

	ts::Node obj_node = node.getChildByFieldName("object");
	if (!obj_node.isNull()) {
		out->object = visitExpression(obj_node, text);
	}

	ts::Node index_node = node.getChildByFieldName("index");
	if (!index_node.isNull()) {
		out->index = visitExpression(index_node, text);
	}

	return out;
}

std::unique_ptr<IdentifierNode> GdmlParser::visitIdentifier(ts::Node node, CharString &text) {
	std::unique_ptr<IdentifierNode> out = std::make_unique<IdentifierNode>();
	out->type = GDMLNodeType::IDENTIFIER_EXPRESSION;
	out->name = node.getText(text);
	out->location = parser::GdmlSourceLocation(node);

	return out;
}

std::unique_ptr<NumberNode> GdmlParser::visitNumber(ts::Node node, CharString &text) {
	std::unique_ptr<NumberNode> out = std::make_unique<NumberNode>();
	out->type = GDMLNodeType::NUMBER_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);
	out->value = node.getText(text);
	out->isFloat = !out->value.is_valid_int();

	return out;
}

std::unique_ptr<BooleanNode> GdmlParser::visitBoolean(ts::Node node, CharString &text) {
	std::unique_ptr<BooleanNode> out = std::make_unique<BooleanNode>();
	out->type = GDMLNodeType::BOOLEAN_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);

	const String &rawValue = node.getText(text);
	out->value = rawValue == "true";

	return out;
}

std::unique_ptr<StringNode> GdmlParser::visitString(ts::Node node, CharString &text) {
	std::unique_ptr<StringNode> out = std::make_unique<StringNode>();
	out->type = GDMLNodeType::STRING_EXPRESSION;
	out->location = parser::GdmlSourceLocation(node);
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

void GdmlParser::pushError(const String &error, const ts::Node &node) {
	parser::GdmlParseError err;
	err.message = error;
	err.location = parser::GdmlSourceLocation(node);
	parse_errors.push_back(err);
}

}

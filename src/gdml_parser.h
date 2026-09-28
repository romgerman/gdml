#pragma once

#include "tree_sitter.h"

#include <memory>
#include <vector>

#include <godot_cpp/variant/string.hpp>

namespace parser {

struct GdmlTextSpan {
	GdmlTextSpan() = default;
	GdmlTextSpan(uint32_t start, uint32_t end) :
			start(start), end(end) {}

	uint32_t getLength() const;

	bool isEmpty() const;

	uint32_t start = 0;
	uint32_t end = 0;
};

struct GdmlLinePosition {
	GdmlLinePosition() = default;
	GdmlLinePosition(uint32_t line, uint32_t column) :
			line(line), column(column) {}

	uint32_t line;
	uint32_t column;
};

struct GdmlSourceLocation {
	GdmlSourceLocation() = default;
	GdmlSourceLocation(ts::Node node);

	godot::String getText(const godot::String &text) const;

	GdmlTextSpan span;
	GdmlLinePosition start;
	GdmlLinePosition end;
};

struct GdmlParseError {
	godot::String toString();

	godot::String message;
	GdmlSourceLocation location;
};

} //namespace parser

namespace godot {

// Base

enum class GdmlNodeType {
	UNKNOWN = 0,
	IDENTIFIER_EXPRESSION,
	NUMBER_EXPRESSION,
	BOOLEAN_EXPRESSION,
	STRING_EXPRESSION,
	ARRAY_EXPRESSION,
	DICTIONARY_EXPRESSION,
	MEMBER_ACCESS_EXPRESSION,
	SIGNAL_EXPRESSION,
	FUNCTION_CALL,
	PROPERTY_DEFINITION,
	RESOURCE_DEFINITION,
	NODE_DEFINITION,
	SIGNAL_DEFINITION,
	DEFINITION,
	SOURCE_FILE
};

struct GdmlNode {
	GdmlNodeType type = GdmlNodeType::UNKNOWN;
};
struct ExpressionNode : public GdmlNode {};
struct StatementNode : public ExpressionNode {};

// Expressions

struct IdentifierNode : public ExpressionNode {
	String name;
};

struct NumberNode : public ExpressionNode {
	String value;
	bool isFloat;

	uint32_t getInteger() const {
		return value.to_int();
	}

	float_t getFloat() const {
		return value.to_float();
	}
};

struct BooleanNode : public ExpressionNode {
	bool value;
};

struct StringNode : public ExpressionNode {
	String value;
};

struct ArrayExpressionNode : public ExpressionNode {
	std::vector<std::unique_ptr<ExpressionNode>> values;
};

struct PropertyDefinitionNode;
struct DictionaryExpressionNode : public ExpressionNode {
	std::vector<std::unique_ptr<PropertyDefinitionNode>> values;
};

struct MemberExpressionNode : public ExpressionNode {
	std::vector<std::unique_ptr<IdentifierNode>> path;
};

struct SignalExpressionNode : public ExpressionNode {
	std::unique_ptr<ExpressionNode> name;
};

struct FunctionCallExpressionNode : public ExpressionNode {
	std::unique_ptr<ExpressionNode> name;
	std::vector<std::unique_ptr<ExpressionNode>> arguments;
};

// Statements

struct PropertyDefinitionNode : public StatementNode {
	std::unique_ptr<ExpressionNode> name;
	std::unique_ptr<ExpressionNode> value;
};

struct ResourceDefinitionNode : public StatementNode {
	std::unique_ptr<IdentifierNode> name;
	std::vector<std::unique_ptr<PropertyDefinitionNode>> properties;
};

struct NodeDefinitionNode : public StatementNode {
	std::unique_ptr<IdentifierNode> name;
	std::vector<std::unique_ptr<PropertyDefinitionNode>> properties;
	std::vector<std::unique_ptr<NodeDefinitionNode>> children;
};

struct SignalDefinitionNode : public StatementNode {
	std::unique_ptr<IdentifierNode> name;
};

struct DefinitionNode : public StatementNode {
	std::unique_ptr<NodeDefinitionNode> node;
	std::unique_ptr<SignalDefinitionNode> signal;
};

struct SourceFileNode : public GdmlNode {
	std::vector<std::unique_ptr<DefinitionNode>> statements;
};

class GdmlParser {
	std::unique_ptr<TSParser, decltype(&ts_parser_delete)> m_parser;
	std::vector<parser::GdmlParseError> parse_errors;

public:
	GdmlParser();
	std::unique_ptr<TSTree, decltype(&ts_tree_delete)> rawParse(const String &text);
	std::unique_ptr<SourceFileNode> parse(const String &text);

	std::vector<parser::GdmlParseError> &getParseErrors();

private:
	std::unique_ptr<DefinitionNode> visitDefinition(ts::Node node, const String &text);
	std::unique_ptr<NodeDefinitionNode> visitNodeDefinition(ts::Node node, const String &text);
	std::unique_ptr<PropertyDefinitionNode> visitProperty(ts::Node node, const String &text);
	std::unique_ptr<ResourceDefinitionNode> visitResource(ts::Node node, const String &text);
	std::unique_ptr<SignalDefinitionNode> visitSignalDefinition(ts::Node node, const String &text);

	std::unique_ptr<ExpressionNode> visitExpression(ts::Node node, const String &text);
	std::unique_ptr<MemberExpressionNode> visitMemberExpression(ts::Node node, const String &text);
	std::unique_ptr<ArrayExpressionNode> visitArrayExpression(ts::Node node, const String &text);
	std::unique_ptr<DictionaryExpressionNode> visitDictionaryExpression(ts::Node node, const String &text);
	std::unique_ptr<SignalExpressionNode> visitSignalExpression(ts::Node node, const String &text);
	std::unique_ptr<FunctionCallExpressionNode> visitFunctionCallExpression(ts::Node node, const String &text);

	std::unique_ptr<IdentifierNode> visitIdentifier(ts::Node node, const String &text);
	std::unique_ptr<NumberNode> visitNumber(ts::Node node, const String &text);
	std::unique_ptr<BooleanNode> visitBoolean(ts::Node node, const String &text);
	std::unique_ptr<StringNode> visitString(ts::Node node, const String &text);

	void pushError(const String &error, const ts::Node &node);
};

} //namespace godot

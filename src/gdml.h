#pragma once

#include <memory>
#include <vector>

#include <godot_cpp/variant/string.hpp>

struct TSParser;
struct TSTree;
struct TSNode;
struct TSPoint;

extern "C" void ts_parser_delete(TSParser* self);
extern "C" void ts_tree_delete(TSTree* self);

namespace ts {

class Node {
private:
    TSNode& m_node;

public:
    Node(TSNode node);

    TSNode raw();
    const char* getType();
    bool isType(const char* typeName);
    bool isNull();
    uint32_t getChildCount();
    uint32_t getNamedChildCount();
    Node getChild(uint32_t index);
    Node getNamedChild(uint32_t index);
    uint32_t getStartByte();
    uint32_t getEndByte();
    TSPoint getStartPoint();
    TSPoint getEndPoint();
    godot::String getText(const godot::String& sourceText);
};

}

namespace godot {

// Base

struct GdmlTextSpan {
    GdmlTextSpan() = default;
    GdmlTextSpan(uint32_t start, uint32_t end): start(start), end(end) {}

    uint32_t getLength() {
        return end - start;
    }

    bool isEmpty() {
        return getLength() == 0;
    }

    uint32_t start = 0;
    uint32_t end = 0;
};

struct GdmlLinePosition {
    GdmlLinePosition() = default;
    GdmlLinePosition(uint32_t line, uint32_t column): line(line), column(column) {}

    uint32_t line;
    uint32_t column;
};

struct GdmlSourceLocation {
    GdmlSourceLocation() = default;
    GdmlSourceLocation(ts::Node node);

    String getText(const String& text);

    GdmlTextSpan span;
    GdmlLinePosition start;
    GdmlLinePosition end;
};

struct GdmlParseError {
    String toString() {
        return message
            + " at "
            + String::num_uint64(location.start.line)
            + ":"
            + String::num_uint64(location.start.column);
    }

    String message;
    GdmlSourceLocation location;
};

enum class GdmlNodeType {
    UNKNOWN = 0,
    IDENTIFIER_EXPRESSION,
    NUMBER_EXPRESSION,
    BOOLEAN_EXPRESSION,
    STRING_EXPRESSION,
    ARRAY_EXPRESSION,
    MEMBER_ACCESS_EXPRESSION,
    PROPERTY_DEFINITION,
    RESOURCE_DEFINITION,
    NODE_DEFINITION,
    DEFINITION,
    SOURCE_FILE
};

struct GdmlNode {
    GdmlNodeType type;
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

    uint32_t getInteger() {
        return value.to_int();
    }

    float_t getFloat() {
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

struct MemberExpressionNode : public ExpressionNode {
    std::vector<std::unique_ptr<IdentifierNode>> path;
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

struct DefinitionNode : public StatementNode {
    std::unique_ptr<NodeDefinitionNode> node;
};

struct SourceFileNode : public GdmlNode {
    std::vector<std::unique_ptr<DefinitionNode>> statements;
};

class Gdml {
    std::unique_ptr<TSParser, decltype(&ts_parser_delete)> m_parser;
    std::vector<GdmlParseError> parse_errors;

public:
    Gdml();
    std::unique_ptr<TSTree, decltype(&ts_tree_delete)> rawParse(const String& text);
    std::unique_ptr<SourceFileNode> parse(const String& text);

    std::vector<GdmlParseError>& getParseErrors();

private:
    std::unique_ptr<DefinitionNode> visitDefinition(ts::Node node, const String& text);
    std::unique_ptr<NodeDefinitionNode> visitNodeDefinition(ts::Node node, const String& text);
    std::unique_ptr<PropertyDefinitionNode> visitProperty(ts::Node node, const String& text);
    std::unique_ptr<ResourceDefinitionNode> visitResource(ts::Node node, const String& text);

    std::unique_ptr<ExpressionNode> visitExpression(ts::Node node, const String& text);
    std::unique_ptr<MemberExpressionNode> visitMemberExpression(ts::Node node, const String& text);
    std::unique_ptr<ArrayExpressionNode> visitArrayExpression(ts::Node node, const String& text);

    std::unique_ptr<IdentifierNode> visitIdentifier(ts::Node node, const String& text);
    std::unique_ptr<NumberNode> visitNumber(ts::Node node, const String& text);
    std::unique_ptr<BooleanNode> visitBoolean(ts::Node node, const String& text);
    std::unique_ptr<StringNode> visitString(ts::Node node, const String& text);

    void pushError(const String& error, ts::Node node);
};

}

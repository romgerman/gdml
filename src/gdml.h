#pragma once

#include <memory>
#include <vector>

#include <godot_cpp/variant/string.hpp>

struct TSParser;
struct TSTree;
struct TSNode;

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
    uint32_t getChildCount();
    uint32_t getNamedChildCount();
    Node getChild(uint32_t index);
    Node getNamedChild(uint32_t index);
    godot::String getText(const godot::String& sourceText);
};

}

namespace godot {

// Base

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

public:
    Gdml();
    std::unique_ptr<TSTree, decltype(&ts_tree_delete)> rawParse(const String& text);
    std::unique_ptr<SourceFileNode> parse(const String& text);

private:
    std::unique_ptr<DefinitionNode> visitDefinition(ts::Node node, const String& text);
    std::unique_ptr<NodeDefinitionNode> visitNodeDefinition(ts::Node node, const String& text);
    std::unique_ptr<PropertyDefinitionNode> visitProperty(ts::Node node, const String& text);
    ResourceDefinitionNode visitResource(TSNode node);

    ExpressionNode visitExpression(TSNode node);
    MemberExpressionNode visitMemberExpression(TSNode node);
    ArrayExpressionNode visitArrayExpression(TSNode node);

    std::unique_ptr<IdentifierNode> visitIdentifier(ts::Node node, const String& text);
    NumberNode visitNumber(TSNode node);
    BooleanNode visitBoolean(TSNode node);
    StringNode visitString(TSNode node);
};

}

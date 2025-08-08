#include "gdml.h"

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-gdml.h>

#include <godot_cpp/core/print_string.hpp>

using namespace godot;

namespace ts {

Node::Node(TSNode node) : m_node(node) {}

TSNode Node::raw() {
    return m_node;
}

const char* Node::getType() {
    return ts_node_type(m_node);
}

bool Node::isType(const char* typeName) {
    return strcmp(getType(), typeName) == 0;
}

bool Node::isNull() {
    return ts_node_is_null(m_node);
}

uint32_t Node::getChildCount() {
    return ts_node_child_count(m_node);
}

uint32_t Node::getNamedChildCount() {
    return ts_node_named_child_count(m_node);
}

Node Node::getChild(uint32_t index) {
    return Node(ts_node_child(m_node, index));
}

Node Node::getNamedChild(uint32_t index) {
    return Node(ts_node_named_child(m_node, index));
}

uint32_t Node::getStartByte() {
    return ts_node_start_byte(m_node);
}

uint32_t Node::getEndByte() {
    return ts_node_end_byte(m_node);
}

TSPoint Node::getStartPoint() {
    return ts_node_start_point(m_node);
}

TSPoint Node::getEndPoint() {
    return ts_node_end_point(m_node);
}

godot::String Node::getText(const godot::String& sourceText) {
    uint32_t start = getStartByte();
    uint32_t end = getEndByte();
    return sourceText.substr(start, end - start);
}

}

GdmlSourceLocation::GdmlSourceLocation(ts::Node node) {
    span = GdmlTextSpan(node.getStartByte(), node.getEndByte());
    TSPoint start_point = node.getStartPoint();
    start = GdmlLinePosition(start_point.row + 1, start_point.column + 1);
    TSPoint end_point = node.getEndPoint();
    end = GdmlLinePosition(end_point.row + 1, end_point.column + 1);
}

String GdmlSourceLocation::getText(const String& text) {
    return text.substr(span.start, span.getLength());
}

Gdml::Gdml() : m_parser(ts_parser_new(), ts_parser_delete) {
    ts_parser_set_language(m_parser.get(), tree_sitter_gdml());
}

std::unique_ptr<TSTree, decltype(&ts_tree_delete)> Gdml::rawParse(const String &text) {
    return std::unique_ptr<TSTree, decltype(&ts_tree_delete)>(
        ts_parser_parse_string(m_parser.get(), nullptr, text.utf8().get_data(), text.length()),
        ts_tree_delete
    );
}

std::unique_ptr<SourceFileNode> Gdml::parse(const String& text) {
    parse_errors.clear();
    auto tree = rawParse(text);
    ts::Node root_node = ts::Node(ts_tree_root_node(tree.get()));

    std::unique_ptr<SourceFileNode> out = std::make_unique<SourceFileNode>();
    out->type = GdmlNodeType::SOURCE_FILE;
    
    for (uint32_t i = 0; i < root_node.getNamedChildCount(); i++) {
        ts::Node child_node = root_node.getNamedChild(i);
        
        if (child_node.isType("definition")) {
            std::unique_ptr<DefinitionNode> def = visitDefinition(child_node.raw(), text);
            out->statements.push_back(std::move(def));
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing source file", child_node);
        }
    }

    return out;
}

std::unique_ptr<DefinitionNode> Gdml::visitDefinition(ts::Node node, const String& text) {
    std::unique_ptr<DefinitionNode> out = std::make_unique<DefinitionNode>();
    out->type = GdmlNodeType::DEFINITION;

    for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
        ts::Node child_node = node.getNamedChild(i);

        if (child_node.isType("node_definition")) {
            out->node = std::move(visitNodeDefinition(child_node, text));
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing definition", child_node);
        }
    }

    return out;
}

std::unique_ptr<NodeDefinitionNode> Gdml::visitNodeDefinition(ts::Node node, const String& text) {
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
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing node definition", child_node);
        }
    }

    return out;
}

std::unique_ptr<PropertyDefinitionNode> Gdml::visitProperty(ts::Node node, const String& text) {
    std::unique_ptr<PropertyDefinitionNode> out = std::make_unique<PropertyDefinitionNode>();
    out->type = GdmlNodeType::PROPERTY_DEFINITION;

    for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
        ts::Node child_node = node.getNamedChild(i);

        if (child_node.isType("identifier")) {
            out->name = visitIdentifier(child_node, text);
        } else if (child_node.isType("member_expression")) {
            out->name = visitMemberExpression(child_node, text);
        } else if (child_node.isType("expression")) {
            out->value = visitExpression(child_node, text);
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing property", child_node);
        }
    }

    return out;
}

ResourceDefinitionNode Gdml::visitResource(TSNode node) {
    return ResourceDefinitionNode();
}

std::unique_ptr<ExpressionNode> Gdml::visitExpression(ts::Node node, const String& text) {
    std::unique_ptr<ExpressionNode> out = std::make_unique<ExpressionNode>();
    out->type = GdmlNodeType::UNKNOWN;

    for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
        ts::Node child_node = node.getNamedChild(i);

        if (child_node.isType("string")) {
            out = visitString(child_node, text);
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing expression", child_node);
        }
    }

    return out;
}

std::unique_ptr<MemberExpressionNode> Gdml::visitMemberExpression(ts::Node node, const String& text) {
    std::unique_ptr<MemberExpressionNode> out = std::make_unique<MemberExpressionNode>();
    out->type = GdmlNodeType::MEMBER_ACCESS_EXPRESSION;

    for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
        ts::Node child_node = node.getNamedChild(i);

        if (child_node.isType("identifier")) {
            out->path.push_back(std::move(visitIdentifier(child_node, text)));
        } else {
            pushError("Unknown node " + String(child_node.getType()) + " while parsing member expression", child_node);
        }
    }

    return out;
}

ArrayExpressionNode Gdml::visitArrayExpression(TSNode node) {
    return ArrayExpressionNode();
}

std::unique_ptr<IdentifierNode> Gdml::visitIdentifier(ts::Node node, const String& text) {
    std::unique_ptr<IdentifierNode> out = std::make_unique<IdentifierNode>();
    out->type = GdmlNodeType::IDENTIFIER_EXPRESSION;
    out->name = node.getText(text);

    return out;
}

NumberNode Gdml::visitNumber(TSNode node) {
    return NumberNode();
}

BooleanNode Gdml::visitBoolean(TSNode node) {
    return BooleanNode();
}

std::unique_ptr<StringNode> Gdml::visitString(ts::Node node, const String& text) {
    std::unique_ptr<StringNode> out = std::make_unique<StringNode>();
    out->type = GdmlNodeType::STRING_EXPRESSION;
    out->value = String();

    for (uint32_t i = 0; i < node.getNamedChildCount(); i++) {
        ts::Node child_node = ts::Node(node.getNamedChild(i));
        out->value += child_node.getText(text);
    }

    return out;
}

std::vector<GdmlParseError>& Gdml::getParseErrors() {
    return parse_errors;
}

void Gdml::pushError(const String& error, ts::Node node) {
    GdmlParseError err;
    err.message = error;
    err.location = GdmlSourceLocation(node);
    parse_errors.push_back(err);
}

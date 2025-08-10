#include "tree_sitter.h"

#include <tree_sitter/api.h>

using namespace ts;

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

#include "tree_sitter.h"

#include <godot_cpp/variant/string.hpp>

using namespace ts;

Node::Node(TSNode node) :
		m_node(node) {}

TSNode Node::raw() const {
	return m_node;
}

const char *Node::getType() const {
	return ts_node_type(m_node);
}

bool Node::isType(const char *typeName) const {
	return strcmp(getType(), typeName) == 0;
}

bool Node::isNull() const {
	return ts_node_is_null(m_node);
}

uint32_t Node::getChildCount() const {
	return ts_node_child_count(m_node);
}

uint32_t Node::getNamedChildCount() const {
	return ts_node_named_child_count(m_node);
}

Node Node::getChild(uint32_t index) const {
	return Node(ts_node_child(m_node, index));
}

Node Node::getNamedChild(uint32_t index) const {
	return Node(ts_node_named_child(m_node, index));
}

Node Node::getChildByFieldName(const godot::String &fieldName) const {
	return Node(ts_node_child_by_field_name(m_node, fieldName.utf8().get_data(), fieldName.utf8().length()));
}

uint32_t Node::getStartByte() const {
	return ts_node_start_byte(m_node);
}

uint32_t Node::getEndByte() const {
	return ts_node_end_byte(m_node);
}

TSPoint Node::getStartPoint() const {
	return ts_node_start_point(m_node);
}

TSPoint Node::getEndPoint() const {
	return ts_node_end_point(m_node);
}

godot::String Node::getText(const godot::String &sourceText) const {
	uint32_t start = getStartByte();
	uint32_t end = getEndByte();
	return sourceText.substr(start, end - start);
}

Node Node::getRootNode(const TSTree *tree) {
	return Node(ts_tree_root_node(tree));
}

Cursor::Cursor(TSNode start_node) {
	m_cursor = ts_tree_cursor_new(start_node);
}

Cursor::Cursor(Node &start_node) {
	m_cursor = ts_tree_cursor_new(start_node.raw());
}

Node Cursor::getCurrentNode() const {
	return ts_tree_cursor_current_node(&m_cursor);
}

bool Cursor::gotoFirstChild() {
	return ts_tree_cursor_goto_first_child(&m_cursor);
}

bool Cursor::gotoNextSibling() {
	return ts_tree_cursor_goto_next_sibling(&m_cursor);
}

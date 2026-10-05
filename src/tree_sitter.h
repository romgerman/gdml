#pragma once

#include <tree_sitter/api.h>

#include <godot_cpp/variant/string.hpp>

namespace ts {

// Just a wapper for TSNode. Lifetime is not handled
class Node {
	TSNode m_node;

public:
	Node(TSNode node);

	TSNode raw() const;
	const char *getType() const;
	bool isType(const char *typeName) const;
	bool isNull() const;
	bool isError() const;
	bool isMissing() const;
	bool hasError() const;
	uint32_t getChildCount() const;
	uint32_t getNamedChildCount() const;
	Node getChild(uint32_t index) const;
	Node getNamedChild(uint32_t index) const;
	Node getChildByFieldName(const godot::CharString &fieldName) const;
	uint32_t getStartByte() const;
	uint32_t getEndByte() const;
	TSPoint getStartPoint() const;
	TSPoint getEndPoint() const;
	godot::String getText(const godot::CharString &sourceText) const;

	bool operator==(Node &right) {
		return ts_node_eq(m_node, right.m_node);
	}

	bool operator!=(Node &right) {
		return !ts_node_eq(m_node, right.m_node);
	}

	static Node getRootNode(const TSTree *tree);
};

class Cursor {
	TSTreeCursor m_cursor;

public:
	Cursor(TSNode start_node);
	Cursor(Node &start_node);
	~Cursor();
	Cursor(const Cursor &) = delete;
	Cursor &operator=(const Cursor &) = delete;

	Node getCurrentNode() const;
	bool gotoFirstChild();
	bool gotoNextSibling();
};

} //namespace ts
#pragma once

#include <tree_sitter/api.h>

namespace godot {
class String;
}

namespace ts {

class Node {
	TSNode m_node;

public:
	Node(TSNode node);

	TSNode raw() const;
	const char *getType() const;
	bool isType(const char *typeName) const;
	bool isNull() const;
	bool isError() const;
	uint32_t getChildCount() const;
	uint32_t getNamedChildCount() const;
	Node getChild(uint32_t index) const;
	Node getNamedChild(uint32_t index) const;
	Node getChildByFieldName(const godot::String &fieldName) const;
	uint32_t getStartByte() const;
	uint32_t getEndByte() const;
	TSPoint getStartPoint() const;
	TSPoint getEndPoint() const;
	godot::String getText(const godot::String &sourceText) const;

	static Node getRootNode(const TSTree *tree);
};

class Cursor {
	TSTreeCursor m_cursor;

public:
	Cursor(TSNode start_node);
	Cursor(Node &start_node);

	Node getCurrentNode() const;
	bool gotoFirstChild();
	bool gotoNextSibling();
};

} //namespace ts
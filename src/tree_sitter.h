#pragma once

#include <tree_sitter/api.h>

#include <godot_cpp/variant/string.hpp>

namespace ts {

class Node {
private:
    TSNode m_node;

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
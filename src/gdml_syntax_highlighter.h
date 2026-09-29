#pragma once

#include "gdml_parser.h"

#include <vector>

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>

namespace ts {
class Node;
}

namespace godot {

class GdmlSyntaxHighlighter : public SyntaxHighlighter {
	GDCLASS(GdmlSyntaxHighlighter, SyntaxHighlighter)

protected:
	static void _bind_methods();

public:
	Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;

	Dictionary get_syntax_highlighting(const godot::String &p_text, int32_t p_line) const;

private:
	mutable GdmlParser _parser;

	std::vector<std::tuple<ts::Node, godot::String, bool>> find_nodes_on_line(const ts::Node &root_node, int32_t line) const;
	godot::Color get_color_for_node(std::tuple<ts::Node, godot::String, bool> &pair) const;
};

} //namespace godot

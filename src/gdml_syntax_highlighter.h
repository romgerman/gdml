#pragma once

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

	Dictionary get_syntax_highlighting(godot::String p_text, int32_t p_line) const;

private:
	std::vector<ts::Node> find_nodes_on_line(ts::Node root_node, int32_t line) const;
	godot::Color get_color_for_node(ts::Node &node) const;
};

} //namespace godot

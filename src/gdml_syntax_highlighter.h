#pragma once

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>

namespace godot {

class GdmlSyntaxHighlighter : public EditorSyntaxHighlighter {
    GDCLASS(GdmlSyntaxHighlighter, SyntaxHighlighter)

protected:
	static void _bind_methods();

public:
	String _get_name() const override;
	PackedStringArray _get_supported_languages() const override;
	Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
};

}

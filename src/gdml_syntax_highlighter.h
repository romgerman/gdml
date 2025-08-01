#pragma once

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>

namespace godot {

class GdmlSyntaxHighlighter : public EditorSyntaxHighlighter {
    GDCLASS(GdmlSyntaxHighlighter, SyntaxHighlighter)

public:
	virtual void _update_cache() override;

	virtual String _get_name() const override;
	virtual PackedStringArray _get_supported_languages() const override;
};

}

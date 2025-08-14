#include "gdml_syntax_highlighter.h"

#include <godot_cpp/classes/text_edit.hpp>

using namespace godot;

void GdmlSyntaxHighlighter::_bind_methods() {}

String GdmlSyntaxHighlighter::_get_name() const {
    return "Gdml";
}

PackedStringArray GdmlSyntaxHighlighter::_get_supported_languages() const {
	PackedStringArray languages;
	languages.push_back("Gdml");
	return languages;
}

Dictionary godot::GdmlSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
	const godot::String text = get_text_edit()->get_line(p_line);
	godot::Dictionary out;

	godot::Dictionary line_out;
	line_out.set("color", Color::named("RED"));
	out.set(0, line_out);

	return out;
}

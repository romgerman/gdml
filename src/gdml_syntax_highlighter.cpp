#include "gdml_syntax_highlighter.h"

using namespace godot;

void GdmlSyntaxHighlighter::_update_cache() {}

String GdmlSyntaxHighlighter::_get_name() const {
    return "Gdml";
}

PackedStringArray GdmlSyntaxHighlighter::_get_supported_languages() const {
	PackedStringArray languages;
	languages.push_back("Gdml");
	return languages;
}

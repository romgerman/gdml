#include "gdml.h"

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-gdml.h>

using namespace godot;

void Gdml::parse(const String& text) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_gdml());
}
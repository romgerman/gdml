#include "gdml.h"

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-gdml.h>

#include <godot_cpp/core/print_string.hpp>

using namespace godot;

void Gdml::parse(const String& text) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_gdml());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, text.utf8().get_data(), text.length());
    TSNode root_node = ts_tree_root_node(tree);

    print_line(ts_node_type(root_node));
}
#include "gdml.h"

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-gdml.h>

using namespace godot;

Gdml::Gdml() : m_parser(ts_parser_new(), ts_parser_delete) {
    ts_parser_set_language(m_parser.get(), tree_sitter_gdml());
}

Gdml::~Gdml() {}

std::unique_ptr<TSTree, decltype(&ts_tree_delete)> Gdml::parse(const String &text) {
    return std::unique_ptr<TSTree, decltype(&ts_tree_delete)>(
        ts_parser_parse_string(m_parser.get(), nullptr, text.utf8().get_data(), text.length()),
        ts_tree_delete
    );
}
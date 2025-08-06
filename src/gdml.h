#pragma once

#include <memory>

#include <godot_cpp/variant/string.hpp>

struct TSParser;
struct TSTree;

extern "C" void ts_parser_delete(TSParser* self);
extern "C" void ts_tree_delete(TSTree *self);

namespace godot {

class Gdml {
public:
    Gdml();
    ~Gdml();
    std::unique_ptr<TSTree, decltype(&ts_tree_delete)> parse(const String& text);

private:
    std::unique_ptr<TSParser, decltype(&ts_parser_delete)> m_parser;
};

}
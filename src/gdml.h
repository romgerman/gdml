#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

class Gdml {
public:
    void parse(const String& text);
};

}
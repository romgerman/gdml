#pragma once

#include "gdml_function.h"

#include <map>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace gdml {

class FunctionCache {
	std::map<godot::String, godot::Ref<GDMLFunction>> m_functions;

public:
	godot::Ref<GDMLFunction> get(godot::String p_name);
};

}

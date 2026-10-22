#pragma once

#include "gdml_decorator.h"

#include <map>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace gdml {

class DecoratorCache {
	std::map<godot::String, godot::Ref<GDMLDecorator>> m_decorators;

public:
	godot::Ref<GDMLDecorator> store(godot::String p_name);
};

}

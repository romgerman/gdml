#pragma once

#include <map>

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>

namespace traversal {

class VariantRegistry {
	static std::map<godot::String, godot::Variant::Type> m_variant_type_map;

	static void initialize_map();

public:
	static bool is_builtin(const godot::String &name);
	static bool get_constant(const godot::String &type, const godot::StringName &name, godot::Variant &out);
	static bool construct(const godot::String &name, godot::Variant &out);
};

}

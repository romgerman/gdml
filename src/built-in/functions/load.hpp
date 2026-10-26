#pragma once

#include "gdml_function.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace gdml {

class GDMLFunction_load : public GDMLFunction {
	GDCLASS(GDMLFunction_load, GDMLFunction)

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("execute", "args"), &GDMLFunction_load::execute);
		ClassDB::bind_method(D_METHOD("get_name"), &GDMLFunction_load::get_name);
	}

public:
	godot::Variant execute(godot::Array p_args) override {
		if (p_args.is_empty()) {
			return godot::Variant();
		} else if (p_args[0].get_type() != godot::Variant::Type::STRING) {
			godot::print_error("Load function can only operate on strings");
			return godot::Variant();
		}

		return godot::ResourceLoader::get_singleton()->load(p_args[0]);
	}

	godot::StringName get_name() override {
		return godot::StringName("load");
	}
};

}

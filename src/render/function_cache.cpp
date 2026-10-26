#include "function_cache.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

namespace gdml {

godot::Ref<GDMLFunction> FunctionCache::get(godot::String p_name) {
	if (m_functions.find(p_name) != m_functions.end()) {
		return m_functions[p_name];
	}

	const String function_classname = "GDMLFunction";

	for (const String &class_name : ClassDB::get_class_list()) {
		if (ClassDB::is_parent_class(class_name, function_classname)) {
			Variant function_variant = ClassDB::instantiate(class_name);
			Object *function_object = function_variant.get_validated_object();
			if (String(function_object->call(StringName("get_name"))) == p_name) {
				Ref<GDMLFunction> fn = Ref<GDMLFunction>(function_object);
				m_functions[p_name] = fn;
				return fn;
			}
		}
	}

	Dictionary class_info;
	Object *function_object = nullptr;

	for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
		if (String(ci.get("base", "")) == function_classname) {
			String path = ci.get("path", "");

			if (path.is_empty()) {
				continue;
			}

			Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
			if (script.is_null()) {
				continue;
			}

			Variant decorator_variant = ClassDB::instantiate(script->get_instance_base_type());
			function_object = decorator_variant.get_validated_object();
			if (!function_object) {
				continue;
			}
			function_object->set_script(script);

			if (!function_object->has_method(StringName("get_name"))) {
				continue;
			}

			if (String(function_object->call(StringName("get_name"))) == p_name) {
				Ref<GDMLFunction> fn = Ref<GDMLFunction>(function_object);
				m_functions[p_name] = fn;
				return fn;
			}
		}
	}

	return nullptr;
}

}

#include "decorator_cache.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

namespace gdml {

godot::Ref<GDMLDecorator> DecoratorCache::store(godot::String p_name) {
	if (m_decorators.find(p_name) != m_decorators.end()) {
		return m_decorators[p_name];
	}

	const String decorator_classname = "GDMLDecorator";

	Dictionary class_info;
	Object *decorator_object = nullptr;

	for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
		if (String(ci.get("base", "")) == decorator_classname) {
			String path = ci.get("path", "");

			if (path.is_empty()) {
				continue;
			}

			Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
			if (script.is_null()) {
				continue;
			}

			Variant decorator_variant = ClassDB::instantiate(script->get_instance_base_type());
			decorator_object = decorator_variant.get_validated_object();
			if (!decorator_object) {
				continue;
			}
			decorator_object->set_script(script);

			if (!decorator_object->has_method(StringName("get_decorator_name"))) {
				continue;
			}

			if (String(decorator_object->call(StringName("get_decorator_name"))) == p_name) {
				return Ref<GDMLDecorator>(decorator_object);
			}
		}
	}

	return nullptr;
}

}

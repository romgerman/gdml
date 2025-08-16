#include "gdml_script_resource_format_loader.h"
#include "gdml_script.h"
#include "gdml_script_language.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

static Ref<GdmlScriptResourceFormatLoader> instance;

void GdmlScriptResourceFormatLoader::_bind_methods() {}

PackedStringArray GdmlScriptResourceFormatLoader::_get_recognized_extensions() const {
	return Array::make("gdml");
}

bool GdmlScriptResourceFormatLoader::_handles_type(const StringName &p_type) const {
	return p_type == Script::get_class_static() || p_type == GdmlScript::get_class_static();
}

String GdmlScriptResourceFormatLoader::_get_resource_type(const String &p_path) const {
	if (p_path.get_extension() == "gdml") {
		return GdmlScript::get_class_static();
	} else {
		return "";
	}
}

bool GdmlScriptResourceFormatLoader::_exists(const String &p_path) const {
	return FileAccess::file_exists(p_path);
}

Variant GdmlScriptResourceFormatLoader::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<GdmlScript> script(GdmlScriptLanguage::get_singleton()->_create_script());
	script->set_path(p_original_path);
	script->set_source_code(FileAccess::get_file_as_string(p_path));
	Error status = script->reload();
	if (status == OK) {
		return script;
	} else {
		return status;
	}
}

void GdmlScriptResourceFormatLoader::register_in_godot() {
	instance.instantiate();
	ResourceLoader::get_singleton()->add_resource_format_loader(instance);
}

void GdmlScriptResourceFormatLoader::unregister_in_godot() {
	ResourceLoader::get_singleton()->remove_resource_format_loader(instance);
	instance.unref();
}

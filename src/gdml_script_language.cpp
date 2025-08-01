#include "gdml_script_language.h"
#include "gdml_script.h"
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

GdmlScriptLanguage *GdmlScriptLanguage::instance = nullptr;

void GdmlScriptLanguage::_bind_methods() {}

String GdmlScriptLanguage::_get_name() const {
	return "Gdml";
}

String GdmlScriptLanguage::_get_type() const {
	return "GdmlScript";
}

String GdmlScriptLanguage::_get_extension() const {
	return "gdml";
}

PackedStringArray GdmlScriptLanguage::_get_recognized_extensions() const {
	return helpers::append_all(PackedStringArray(),
		"gdml"
	);
}

void GdmlScriptLanguage::_init() {}

void GdmlScriptLanguage::_finish() {}

void GdmlScriptLanguage::_frame() {}

void GdmlScriptLanguage::_thread_enter() {}

void GdmlScriptLanguage::_thread_exit() {}

void GdmlScriptLanguage::_reload_all_scripts() {}

String GdmlScriptLanguage::_validate_path(const String &p_path) const {
  return "";
}

Dictionary GdmlScriptLanguage::_validate(
    const String &p_script, const String &p_path, bool p_validate_functions,
    bool p_validate_errors, bool p_validate_warnings,
    bool p_validate_safe_lines) const {
  Dictionary result;
  result["valid"] = true;
  return result;
}

bool GdmlScriptLanguage::_supports_builtin_mode() const { return false; }

bool GdmlScriptLanguage::_can_inherit_from_file() const { return false; }

bool GdmlScriptLanguage::_is_using_templates() { return false; }

bool GdmlScriptLanguage::_overrides_external_editor() { return false; }

bool GdmlScriptLanguage::_supports_documentation() const {
  return false;
}

PackedStringArray GdmlScriptLanguage::_get_string_delimiters() const {
    return helpers::append_all(PackedStringArray(),
        "\" \""
    );
}

PackedStringArray godot::GdmlScriptLanguage::_get_comment_delimiters() const {
    return helpers::append_all(PackedStringArray(),
        "#"
    );
}

PackedStringArray GdmlScriptLanguage::_get_reserved_words() const {
  return PackedStringArray();
}

Ref<Script> GdmlScriptLanguage::_make_template(const String &_template, const String &class_name, const String &base_class_name) const {
	Ref<GdmlScript> script = memnew(GdmlScript);
	script->set_source_code("test code");
	return script;
}

Object *godot::GdmlScriptLanguage::_create_script() const {
    return memnew(GdmlScript);
}

GdmlScriptLanguage* GdmlScriptLanguage::get_singleton() {
	return instance;
}

GdmlScriptLanguage* GdmlScriptLanguage::get_or_create_singleton() {
	if (!instance) {
		instance = memnew(GdmlScriptLanguage);
		Engine::get_singleton()->register_script_language(instance);
	}
	return instance;
}

void GdmlScriptLanguage::delete_singleton() {
	if (instance) {
		Engine::get_singleton()->unregister_script_language(instance);
		memdelete(instance);
		instance = nullptr;
	}
}
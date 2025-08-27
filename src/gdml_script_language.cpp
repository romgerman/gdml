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
			"gdml");
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

Error GdmlScriptLanguage::_open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column) {
	return OK;
}

ScriptLanguage::ScriptNameCasing GdmlScriptLanguage::_preferred_file_name_casing() const {
	return SCRIPT_NAME_CASING_AUTO;
}

Dictionary GdmlScriptLanguage::_complete_code(const String &p_code,
		const String &p_path,
		Object *p_owner) const {
	Dictionary out;

	// Array options;
	// out["options"] = options;

	int64_t pos = p_code.find(String::chr(char32_t(0xFFFF)));

	out["result"] = Error::ERR_UNAVAILABLE;
	out["force"] = Variant(false);
	out["call_hint"] = Variant(false);

	return out;
}

Dictionary GdmlScriptLanguage::_lookup_code(const String &p_code,
		const String &p_symbol,
		const String &p_path,
		Object *p_owner) const {
	Dictionary out;

	if (ClassDB::class_exists(p_symbol)) {
		out["result"] = Error::OK;
		out["type"] = LookupResultType::LOOKUP_RESULT_CLASS;
		out["class_name"] = p_symbol;
		return out;
	}

	out["result"] = Error::ERR_UNAVAILABLE;
	out["type"] = Variant::Type::NIL;

	return out;
}

bool GdmlScriptLanguage::_supports_builtin_mode() const {
	return false;
}

bool GdmlScriptLanguage::_can_inherit_from_file() const {
	return false;
}

bool GdmlScriptLanguage::_is_using_templates() {
	return false;
}

bool GdmlScriptLanguage::_overrides_external_editor() {
	return false;
}

bool GdmlScriptLanguage::_supports_documentation() const {
	return false;
}

bool GdmlScriptLanguage::_has_named_classes() const {
	return false;
}

bool GdmlScriptLanguage::_handles_global_class_type(const String &p_type) const {
	return p_type == _get_type();
}

PackedStringArray GdmlScriptLanguage::_get_string_delimiters() const {
	return helpers::append_all(PackedStringArray(),
			"\" \"");
}

PackedStringArray GdmlScriptLanguage::_get_comment_delimiters() const {
	return helpers::append_all(PackedStringArray(),
			"#");
}

PackedStringArray GdmlScriptLanguage::_get_reserved_words() const {
	return PackedStringArray();
}

TypedArray<Dictionary> GdmlScriptLanguage::_get_built_in_templates(const StringName &p_object) const {
	return {};
}

TypedArray<Dictionary> GdmlScriptLanguage::_get_public_functions() const {
	return {};
}

Dictionary GdmlScriptLanguage::_get_public_constants() const {
	return {};
}

TypedArray<Dictionary> GdmlScriptLanguage::_get_public_annotations() const {
	return {};
}

Dictionary GdmlScriptLanguage::_get_global_class_name(const String &p_path) const {
	return {};
}

Ref<Script> GdmlScriptLanguage::_make_template(const String &_template, const String &class_name, const String &base_class_name) const {
	Ref<GdmlScript> script = memnew(GdmlScript);
	script->set_source_code("Control {\n\n}");
	return script;
}

Object *GdmlScriptLanguage::_create_script() const {
	return memnew(GdmlScript);
}

GdmlScriptLanguage *GdmlScriptLanguage::get_singleton() {
	return instance;
}

GdmlScriptLanguage *GdmlScriptLanguage::get_or_create_singleton() {
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
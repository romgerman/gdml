#include "gdml_script.h"

#include "gdml_script_language.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GdmlScript::_bind_methods() {}

Error GdmlScript::_reload(bool p_keep_state) {
	return OK;
}

bool GdmlScript::_editor_can_reload_from_file() {
	return true;
}

StringName GdmlScript::_get_global_name() const {
	return "GlobalName";
}

StringName GdmlScript::_get_instance_base_type() const {
	return "";
}

bool GdmlScript::_has_static_method(const StringName &p_method) const {
	return false;
}

Ref<Script> GdmlScript::_get_base_script() const {
	return {};
}

ScriptLanguage *GdmlScript::_get_language() const {
	return GdmlScriptLanguage::get_singleton();
}

String GdmlScript::_get_source_code() const {
	return source_code;
}

void GdmlScript::_set_source_code(const String &p_code) {
	source_code = p_code;
}

bool GdmlScript::_has_source_code() const {
	return !source_code.is_empty();
}

void GdmlScript::_update_exports() {}

bool GdmlScript::_is_tool() const {
	return false;
}

bool GdmlScript::_is_abstract() const {
	return false;
}

void GdmlScript::_placeholder_erased(void *p_placeholder) {}

bool GdmlScript::_can_instantiate() const {
	return true;
}

TypedArray<Dictionary> GdmlScript::_get_documentation() const {
	return {};
}

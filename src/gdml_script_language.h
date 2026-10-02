#pragma once

#include <godot_cpp/classes/script_language_extension.hpp>

namespace godot {

class GdmlScriptLanguage : public ScriptLanguageExtension {
	GDCLASS(GdmlScriptLanguage, ScriptLanguageExtension)

private:
	static GdmlScriptLanguage *instance;

protected:
	static void _bind_methods();

public:
	String _get_name() const override;
	String _get_type() const override;
	String _get_extension() const override;
	PackedStringArray _get_recognized_extensions() const override;

	void _init() override;
	void _finish() override;
	void _frame() override;

	void _thread_enter() override;
	void _thread_exit() override;
	void _reload_all_scripts() override;
	void _reload_scripts(const Array &p_scripts, bool p_soft_reload) override;

	String _validate_path(const String &p_path) const override;
	Dictionary _validate(const String &p_script, const String &p_path, bool p_validate_functions, bool p_validate_errors, bool p_validate_warnings, bool p_validate_safe_lines) const override;
	Error _open_in_external_editor(const Ref<Script> &p_script, int32_t p_line, int32_t p_column) override;
	ScriptLanguage::ScriptNameCasing _preferred_file_name_casing() const override;

	Dictionary _complete_code(const String &p_code, const String &p_path, Object *p_owner) const override;
	Dictionary _lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner) const override;

	bool _supports_builtin_mode() const override;
	bool _can_inherit_from_file() const override;
	bool _is_using_templates() override;
	bool _overrides_external_editor() override;
	bool _supports_documentation() const override;
	bool _has_named_classes() const override;
	bool _handles_global_class_type(const String &p_type) const override;

	PackedStringArray _get_string_delimiters() const override;
	PackedStringArray _get_comment_delimiters() const override;
	PackedStringArray _get_reserved_words() const override;

	TypedArray<Dictionary> _get_built_in_templates(const StringName &p_object) const override;
	TypedArray<Dictionary> _get_public_functions() const override;
	Dictionary _get_public_constants() const override;
	TypedArray<Dictionary> _get_public_annotations() const override;
	Dictionary _get_global_class_name(const String &p_path) const override;

	Ref<Script> _make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	Object *_create_script() const override;

	static GdmlScriptLanguage *get_singleton();
	static GdmlScriptLanguage *get_or_create_singleton();
	static void delete_singleton();
};

} //namespace godot
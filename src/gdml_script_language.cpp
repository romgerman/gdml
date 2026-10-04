#include "gdml_script_language.h"

#include "gdml_script.h"
#include "gdml_parser.h"

#include <vector>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace godot {

GdmlScriptLanguage *GdmlScriptLanguage::instance = nullptr;

void GdmlScriptLanguage::get_directory_contents(EditorFileSystemDirectory *p_dir, std::vector<String> &out_files) const {
	for (int32_t i = 0; i < p_dir->get_file_count(); i++) {
		if (ClassDB::is_parent_class(p_dir->get_file_type(i), "PackedScene")
			// || ClassDB::is_parent_class(p_dir->get_file_type(i), "GdmlScript")
			|| ClassDB::is_parent_class(p_dir->get_file_type(i), "GDScript")) {
			out_files.push_back(p_dir->get_file_path(i));
		}
	}

	for (int32_t i = 0; i < p_dir->get_subdir_count(); i++) {
		get_directory_contents(p_dir->get_subdir(i), out_files);
	}
}

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
	return helpers::append_all(PackedStringArray(), "gdml");
}

void GdmlScriptLanguage::_init() {}

void GdmlScriptLanguage::_finish() {}

void GdmlScriptLanguage::_frame() {}

void GdmlScriptLanguage::_thread_enter() {}

void GdmlScriptLanguage::_thread_exit() {}

void GdmlScriptLanguage::_reload_all_scripts() {}

void GdmlScriptLanguage::_reload_scripts(const Array &p_scripts, bool p_soft_reload) {

}

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

Dictionary GdmlScriptLanguage::_complete_code(const String &p_code, const String &p_path, Object *p_owner) const {
	int64_t cursor_pos = p_code.find(String::chr(char32_t(0xFFFF)));
	
	GdmlParser parser;
	std::unique_ptr<SourceFileNode> root_node = parser.parse(p_code);

	GdmlNode *context_node = nullptr;
	std::vector<GdmlNode *> node_stack;
	node_stack.push_back(root_node.get());
	
	while (!node_stack.empty()) {
		GdmlNode *current_node = node_stack.back();
		node_stack.pop_back();

		if (current_node->location.span.start > cursor_pos || current_node->location.span.end < cursor_pos) {
			continue;
		}

		if (current_node->type == GDMLNodeType::SOURCE_FILE) {
			SourceFileNode *node = (SourceFileNode *)current_node;
			for (const std::unique_ptr<DefinitionNode> &statement : node->statements) {
				node_stack.push_back(statement.get());
			}
			for (const std::unique_ptr<ImportStatementNode> &statement : node->imports) {
				node_stack.push_back(statement.get());
			}
			context_node = current_node;
		} else if (current_node->type == GDMLNodeType::DEFINITION) {
			DefinitionNode *node = (DefinitionNode *)current_node;
			node_stack.push_back(node->statement.get());
			context_node = current_node;
		} else if (current_node->type == GDMLNodeType::IMPORT) {
			context_node = current_node;
		}
	}
	
	Dictionary out;
	Array options;

	if (context_node) {
		if (context_node->type == GDMLNodeType::IMPORT) {
			std::vector<String> files;
			get_directory_contents(EditorInterface::get_singleton()->get_resource_filesystem()->get_filesystem(), files);

			for (String &file_path : files) {
				Dictionary complete_option;
				complete_option["kind"] = CodeCompletionKind::CODE_COMPLETION_KIND_FILE_PATH;
				complete_option["display"] = file_path;
				complete_option["insert_text"] = file_path;
				complete_option["default_value"] = Variant();
				complete_option["location"] = CodeCompletionLocation::LOCATION_LOCAL;
				
				options.push_back(complete_option);
			}
		} else if (context_node->type == GDMLNodeType::DEFINITION) {
			DefinitionNode *def_node = (DefinitionNode *)context_node;
			
			if (def_node->statement->type == GDMLNodeType::NODE_DEFINITION) {
				NodeDefinitionNode *node = (NodeDefinitionNode *)def_node->statement.get();
				String node_name = node->name->name;
		
				if (ClassDB::class_exists(node_name)) {
					TypedArray<Dictionary> props = ClassDB::class_get_property_list(node_name, false);
		
					for (Dictionary prop_info : props) {
						Dictionary complete_option;
						complete_option["kind"] = CodeCompletionKind::CODE_COMPLETION_KIND_MEMBER;
						complete_option["display"] = prop_info.get("name", "");
						complete_option["insert_text"] = prop_info.get("name", "");
						complete_option["default_value"] = Variant();
						complete_option["location"] = CodeCompletionLocation::LOCATION_LOCAL;
					
						options.push_back(complete_option);
					}
				}

			}
		}
	}

	out["options"] = options;
	out["result"] = Error::OK;
	out["force"] = Variant(false);
	out["call_hint"] = String();

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

}

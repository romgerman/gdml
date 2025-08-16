#include "gdml_editor_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;

void GdmlEditorPlugin::_bind_methods() {}

void GdmlEditorPlugin::_enter_tree() {
	if (ResourceLoader::get_singleton()->exists("res://addons/gdml/gdml_syntax_highlighter.gd")) {
		Ref<Script> highlighter_script = ResourceLoader::get_singleton()->load("res://addons/gdml/gdml_syntax_highlighter.gd");
		Ref<EditorSyntaxHighlighter> default_highlighter = memnew(EditorSyntaxHighlighter);
		default_highlighter->set_script(highlighter_script);
		m_highlighter = default_highlighter;
		get_editor_interface()->get_script_editor()->register_syntax_highlighter(default_highlighter);
	} else {
		print_error("Couldn't load GDML syntax highlighter script. Does it exist?");
	}
}

void GdmlEditorPlugin::_exit_tree() {
	if (m_highlighter.is_valid()) {
		get_editor_interface()->get_script_editor()->unregister_syntax_highlighter(m_highlighter);
		m_highlighter.unref();
	}
}

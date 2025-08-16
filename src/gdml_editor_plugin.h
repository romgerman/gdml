#pragma once

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_syntax_highlighter.hpp>

namespace godot {

class GdmlEditorPlugin : public godot::EditorPlugin {
	GDCLASS(GdmlEditorPlugin, godot::EditorPlugin);

	Ref<EditorSyntaxHighlighter> m_highlighter;

protected:
	static void _bind_methods();

public:
	void _enter_tree() override;
	void _exit_tree() override;
};

}
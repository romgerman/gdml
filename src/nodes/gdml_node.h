#pragma once

#include <godot_cpp/classes/control.hpp>

#include "gdml_atom.h"

namespace gdml {
	class GDMLRenderer;
	class GDMLResult;
}

namespace godot {

class GdmlScript;

class GDMLNode : public Control {
	GDCLASS(GDMLNode, Control)

private:
	Ref<GdmlScript> _markup;
	Ref<gdml::GDMLRenderer> _renderer;
	Ref<gdml::GDMLResult> _renderer_result;
	bool m_render_in_editor = true;
	bool m_nodes_preview_enabled = false;

	void render_in_editor();
	void render_in_game();

protected:
	static void _bind_methods();

public:
	GDMLNode();
	~GDMLNode();

	void _notification(int p_what);
	
	void on_markup_changed();

	// API

	void set_markup(const Ref<GdmlScript> &p_markup);
	Ref<GdmlScript> get_markup();

	void set_render_in_editor(bool p_enabled);
	bool get_render_in_editor();

	void set_is_nodes_preview_enabled(bool p_enabled);
	bool get_is_nodes_preview_enabled();

	Ref<reactive::GDMLAtom> get_variable(String p_name);
	Variant get_variable_value(String p_name);
	void set_variable_value(String p_name, Variant value);

	godot::Error connect_signal(godot::String p_signal_name, godot::Callable p_callable, uint32_t p_flags = 0U);

	void use_effect(Callable p_fn);
};

}

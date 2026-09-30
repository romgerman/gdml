#pragma once

#include <godot_cpp/classes/control.hpp>

#include "gdml_atom.h"

namespace godot {

class GdmlScript;
class GDMLRenderer;
class GDMLResult;

class GDMLNode : public Control {
	GDCLASS(GDMLNode, Control)

private:
	Ref<GdmlScript> _markup;
	Ref<GDMLRenderer> _renderer;
	Ref<godot::GDMLResult> _renderer_result;
	bool m_render_in_editor = true;

protected:
	static void _bind_methods();

public:
	GDMLNode();
	~GDMLNode();

	void _ready() override;

	void _notification(int p_what);

	void set_markup(const Ref<GdmlScript> &p_markup);
	Ref<GdmlScript> get_markup();

	void set_render_in_editor(bool p_enabled);
	bool get_render_in_editor();

	godot::Ref<reactive::GDMLAtom> get_variable(godot::StringName p_name);
};

}

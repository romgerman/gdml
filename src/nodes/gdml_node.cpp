#include "gdml_node.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/script_language.hpp>

#include "gdml_script.h"
#include "render/gdml_renderer.h"

using namespace godot;

void GDMLNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_markup", "p_markup"), &GDMLNode::set_markup);
	ClassDB::bind_method(D_METHOD("get_markup"), &GDMLNode::get_markup);

	ADD_PROPERTY(
		PropertyInfo(Variant::OBJECT, "markup", PROPERTY_HINT_RESOURCE_TYPE, GdmlScript::get_class_static()),
		"set_markup",
		"get_markup"
	);

	ClassDB::bind_method(D_METHOD("set_render_in_editor", "p_enabled"), &GDMLNode::set_render_in_editor);
	ClassDB::bind_method(D_METHOD("get_render_in_editor"), &GDMLNode::get_render_in_editor);

	ADD_PROPERTY(
		PropertyInfo(Variant::BOOL, "render_in_editor"),
		"set_render_in_editor",
		"get_render_in_editor"
	);

	ClassDB::bind_method(D_METHOD("get_variable", "name"), &GDMLNode::get_variable);
}

GDMLNode::GDMLNode() {
	_renderer = Ref<GdmlRenderer>(memnew(GdmlRenderer));
}

GDMLNode::~GDMLNode() {
}

void godot::GDMLNode::_ready() {
	if (_markup.is_valid()) {
		_renderer_result = _renderer->renderMarkup(_markup.ptr(), Dictionary());
		for (godot::Variant item : _renderer_result->nodes) {
			add_child(cast_to<Control>(item));
		}
	}
}

void godot::GDMLNode::_notification(int p_what) {
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE: {
		if (Engine::get_singleton()->is_editor_hint() && m_render_in_editor) {
			print_line("RUNNING THIS IN THE EDITOR");
			if (_markup.is_valid()) {
				godot::Ref<godot::GDMLResult> result = _renderer->renderMarkup(_markup.ptr(), Dictionary());
				for (godot::Variant item : result->nodes) {
					add_child(cast_to<Control>(item));
				}
			}
		}
	} break;
	}
}

void GDMLNode::set_markup(const Ref<GdmlScript> &p_markup) {
	_markup = p_markup;
}

Ref<GdmlScript> GDMLNode::get_markup() {
	return _markup;
}

void godot::GDMLNode::set_render_in_editor(bool p_enabled) {
	m_render_in_editor = p_enabled;
}

bool godot::GDMLNode::get_render_in_editor() {
	return m_render_in_editor;
}

godot::Ref<reactive::GDMLAtom> godot::GDMLNode::get_variable(godot::StringName p_name) {
	return godot::Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
}

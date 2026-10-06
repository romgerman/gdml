#include "gdml_node.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/script_language.hpp>

#include "gdml_script.h"
#include "render/gdml_renderer.h"

using namespace godot;

namespace godot {

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

	ClassDB::bind_method(D_METHOD("use_effect", "fn"), &GDMLNode::use_effect);
}

GDMLNode::GDMLNode() {
	_renderer = Ref<GDMLRenderer>(memnew(GDMLRenderer));
}

GDMLNode::~GDMLNode() {
}

void GDMLNode::_ready() {}

void GDMLNode::_notification(int p_what) {
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE: {
		if (Engine::get_singleton()->is_editor_hint() && m_render_in_editor) {
			render_in_editor();
		}

		if (!Engine::get_singleton()->is_editor_hint()) {
			render_in_game();
		}
	} break;
	}
}

void GDMLNode::render_in_editor() {
	if (_markup.is_valid()) {
		for (Variant item : get_children()) {
			cast_to<Node>(item)->queue_free();
		}

		_renderer_result = _renderer->renderMarkup(this, _markup.ptr(), false);
	}
}

void GDMLNode::render_in_game() {
	if (_markup.is_valid()) {
		_renderer_result = _renderer->renderMarkup(this, _markup.ptr(), true);
		on_signals_changed();
		_renderer_result->connect("signals_changed", callable_mp(this, &GDMLNode::on_signals_changed));
	}
}

void GDMLNode::on_markup_changed() {
	if (Engine::get_singleton()->is_editor_hint()) {
		render_in_editor();
	}
}

void GDMLNode::on_bubble_signal0(Variant target, String signal_name) {
	emit_signal(signal_name, target);
}

void GDMLNode::on_bubble_signal1(Variant target, Variant arg1, String signal_name) {
	emit_signal(signal_name, target, arg1);
}

void GDMLNode::on_bubble_signal2(Variant target, Variant arg1, Variant arg2, String signal_name) {
	emit_signal(signal_name, target, arg1, arg2);
}

void GDMLNode::on_signals_changed() {
	for (Variant signal_name : _renderer_result->get_signals()) {
		for (Variant item : _renderer_result->get_signal_list()) {
			Dictionary signal_info = item;
			if (signal_info["name"] == String(signal_name)) {
				Array args = signal_info["args"];
				print_line("HERE: ", args.size());
				switch (args.size())
				{
				case 1: {
					if (_renderer_result->is_connected(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal0))) {
						continue;
					} else {
						_renderer_result->connect(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal0).bind(signal_name));
					}
				} break;
				case 2: {
					if (_renderer_result->is_connected(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal1))) {
						continue;
					} else {
						_renderer_result->connect(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal1).bind(signal_name));
					}
				} break;
				case 3: {
					if (_renderer_result->is_connected(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal2))) {
						continue;
					} else {
						_renderer_result->connect(signal_name, callable_mp(this, &GDMLNode::on_bubble_signal2).bind(signal_name));
					}
				} break;
				}
			}
		}
		
		if (!has_user_signal(signal_name)) {
			add_user_signal(signal_name);
		}
	}
}

void GDMLNode::set_markup(const Ref<GdmlScript> &p_markup) {
	_markup = p_markup;
	_markup->connect("changed", callable_mp(this, &GDMLNode::on_markup_changed));
}

Ref<GdmlScript> GDMLNode::get_markup() {
	return _markup;
}

void GDMLNode::set_render_in_editor(bool p_enabled) {
	m_render_in_editor = p_enabled;
}

bool GDMLNode::get_render_in_editor() {
	return m_render_in_editor;
}

Ref<reactive::GDMLAtom> GDMLNode::get_variable(String p_name) {
	return _renderer_result->get_variable(p_name);
}

void GDMLNode::use_effect(Callable p_fn) {

}

}

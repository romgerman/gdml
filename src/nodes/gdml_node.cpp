#include "gdml_node.h"

#include <godot_cpp/classes/script_language.hpp>

#include "gdml_script.h"
#include "render/gdml_renderer.h"

using namespace godot;

void GDMLNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_markup"), &GDMLNode::get_markup);
	ClassDB::bind_method(D_METHOD("set_markup", "p_markup"), &GDMLNode::set_markup);

	ADD_PROPERTY(
		PropertyInfo(Variant::OBJECT, "markup", PROPERTY_HINT_RESOURCE_TYPE, GdmlScript::get_class_static()),
		"set_markup",
		"get_markup"
	);
}

GDMLNode::GDMLNode() {
	_renderer = Ref<GdmlRenderer>(memnew(GdmlRenderer));
}

GDMLNode::~GDMLNode() {
}

void godot::GDMLNode::_ready() {
	if (_markup.is_valid()) {
		Variant out = _renderer->renderScript(_markup.ptr(), Dictionary());
		add_child(cast_to<Control>(out));
	}
}

void GDMLNode::set_markup(const Ref<GdmlScript> &p_markup) {
	_markup = p_markup;
}

Ref<GdmlScript> GDMLNode::get_markup() {
	return _markup;
}

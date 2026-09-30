#include "gdml_renderer.h"

#include "tree_renderer_visitor.h"

#include <assert.h>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

void godot::GDMLResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_nodes"), &GDMLResult::get_nodes);
	ClassDB::bind_method(D_METHOD("get_variable_list"), &GDMLResult::get_variable_list);
}

godot::Array godot::GDMLResult::get_nodes() {
	return nodes;
}

godot::Dictionary godot::GDMLResult::get_variable_list() {
	return variables;
}

godot::Ref<reactive::GDMLAtom> godot::GDMLResult::get_variable(godot::String &p_name) {
	return godot::Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
}

void GDMLRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("renderString", "source", "data"), &GDMLRenderer::renderString, DEFVAL(godot::Dictionary()));
	ClassDB::bind_method(D_METHOD("renderMarkup", "markup_script", "data"), &GDMLRenderer::renderMarkup, DEFVAL(godot::Dictionary()));
}

GDMLRenderer::GDMLRenderer() :
		m_parser(std::make_unique<GdmlParser>()) {}

Ref<GDMLResult> GDMLRenderer::renderString(const String &source, const godot::Dictionary &data) {
	std::unique_ptr<godot::SourceFileNode> result = m_parser->parse(source);

	print_line(traversal::GdmlPrinterVisitor().visit(*result));

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}
	traversal::TreeRendererVisitor visitor;
	traversal::TreeRendererVisitorResult visitor_result = visitor.visit(*result);

	Ref<GDMLResult> out = Ref<GDMLResult>(memnew(GDMLResult));

	for (godot::Variant node : visitor_result.nodes) {
		out->nodes.push_back(node);
	}

	for (std::pair<godot::String, godot::Variant> var : visitor_result.variables) {
		out->variables.set(var.first, var.second);
	}

	return out;
}

Ref<GDMLResult> godot::GDMLRenderer::renderMarkup(GdmlScript *script, const godot::Dictionary &data) {
	return renderString(script->get_source_code(), data);
}

#include "gdml_renderer.h"

#include "tree_renderer_visitor.h"

#include <assert.h>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

void GdmlRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("renderString", "source", "data"), &GdmlRenderer::renderString, DEFVAL(godot::Dictionary()));
	ClassDB::bind_method(D_METHOD("renderScript", "script", "data"), &GdmlRenderer::renderScript, DEFVAL(godot::Dictionary()));
}

GdmlRenderer::GdmlRenderer() :
		m_parser(std::make_unique<GdmlParser>()) {}

Variant GdmlRenderer::renderString(const String &source, const godot::Dictionary &data) {
	std::unique_ptr<godot::SourceFileNode> result = m_parser->parse(source);

	//print_line(traversal::GdmlPrinterVisitor().visit(*result.get()));

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}
	traversal::TreeRendererVisitor visitor;
	visitor.setRendererData(data);
	return visitor.visit(*result);
}

Variant godot::GdmlRenderer::renderScript(GdmlScript *script, const godot::Dictionary &data) {
	return renderString(script->get_source_code(), data);
}

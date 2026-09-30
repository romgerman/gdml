#include "gdml_renderer.h"

#include "tree_renderer_visitor.h"

#include <assert.h>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

namespace godot {

void godot::GDMLResult::_bind_methods() {
	
	ClassDB::bind_method(D_METHOD("get_nodes"), &GDMLResult::get_nodes);
	ClassDB::bind_method(D_METHOD("get_variable_list"), &GDMLResult::get_variable_list);
	ClassDB::bind_method(D_METHOD("get_variable", "name"), &GDMLResult::get_variable);
	
	ClassDB::bind_method(D_METHOD("get_signals"), &GDMLResult::get_signals);
}

void GDMLResult::connect_signal(String signal_name, ObjectID target, StringName target_signal) {
	Object *target_object = ObjectDB::get_instance(target);

	if (target_object) {
		target_object->connect(target_signal, callable_mp(this, &GDMLResult::on_defined_signal).bind(signal_name));
		if (!has_user_signal(signal_name)) {
			add_user_signal(signal_name);
		}
	}
}

void GDMLResult::on_defined_signal(String signal_name) {
	emit_signal(signal_name);
}

godot::Array godot::GDMLResult::get_nodes() {
	return nodes;
}

godot::Array godot::GDMLResult::get_variable_list() {
	return variables.keys();
}

godot::Ref<reactive::GDMLAtom> godot::GDMLResult::get_variable(godot::String p_name) {
	if (variables.has(p_name)) {
		return variables[p_name];
	} else {
		return nullptr;
	}
}

godot::Array GDMLResult::get_signals() {
	return signals;
}

void GDMLRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("renderString", "source"), &GDMLRenderer::renderString);
	ClassDB::bind_method(D_METHOD("renderMarkup", "markup_script"), &GDMLRenderer::renderMarkup);
}

GDMLRenderer::GDMLRenderer() :
		m_parser(std::make_unique<GdmlParser>()), m_visitor(std::make_unique<traversal::TreeRendererVisitor>()) {}

Ref<GDMLResult> GDMLRenderer::renderString(const String &source) {
	std::unique_ptr<godot::SourceFileNode> result = m_parser->parse(source);

	print_line(traversal::GdmlPrinterVisitor().visit(*result));

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}
	traversal::TreeRendererVisitorResult visitor_result = m_visitor->visit(*result);

	Ref<GDMLResult> out = Ref<GDMLResult>(memnew(GDMLResult));

	for (godot::Variant node : visitor_result.nodes) {
		out->nodes.push_back(node);
	}

	out->variables = visitor_result.variables;

	for (std::pair<String, traversal::TreeRendererVisitorContext::SignalTarget> pair : visitor_result.signals) {
		for (std::pair<Variant, std::set<godot::StringName>> target_pair : pair.second) {
			for (godot::StringName target_signal : target_pair.second) {
				out->connect_signal(pair.first, target_pair.first, target_signal);
			}
		}

		out->signals.push_back(pair.first);
	}

	return out;
}

Ref<GDMLResult> godot::GDMLRenderer::renderMarkup(GdmlScript *script) {
	return renderString(script->get_source_code());
}

}
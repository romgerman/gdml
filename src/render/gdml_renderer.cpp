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

void GDMLResult::_bind_methods() {
	
	ClassDB::bind_method(D_METHOD("get_nodes"), &GDMLResult::get_nodes);
	ClassDB::bind_method(D_METHOD("get_variable_list"), &GDMLResult::get_variable_list);
	ClassDB::bind_method(D_METHOD("get_variable", "name"), &GDMLResult::get_variable);
	
	ClassDB::bind_method(D_METHOD("get_signals"), &GDMLResult::get_signals);
}

GDMLResult::GDMLResult() {
	add_user_signal("signals_changed");
}

void GDMLResult::add_variable(String &p_name, Ref<reactive::GDMLAtom> atom) {
	m_variables[p_name] = atom;
}

void GDMLResult::connect_signal(String signal_name, ObjectID target, StringName target_signal) {
	Object *target_object = ObjectDB::get_instance(target);

	if (target_object) {
		for (Variant item : target_object->get_signal_list()) {
			Dictionary signal_info = item;
			if (signal_info["name"] == String(target_signal)) {
				Array args = signal_info["args"];
				switch (args.size())
				{
				case 0: {
					if (target_object->is_connected(target_signal, callable_mp(this, &GDMLResult::on_defined_signal0))) {
						return;
					} else {
						target_object->connect(target_signal, callable_mp(this, &GDMLResult::on_defined_signal0).bind(signal_name, target_object));
					}

					if (!has_user_signal(signal_name)) {
						Dictionary target_arg;
						target_arg["name"] = "target";
						target_arg["type"] = Variant::OBJECT;
						add_user_signal(signal_name, Array({
							target_arg
						}));
						m_signals.push_back(signal_name);
					}
				} break;
				case 1: {
					if (target_object->is_connected(target_signal, callable_mp(this, &GDMLResult::on_defined_signal1))) {
						return;
					} else {
						target_object->connect(target_signal, callable_mp(this, &GDMLResult::on_defined_signal1).bind(signal_name, target_object));
					}

					if (!has_user_signal(signal_name)) {
						Dictionary target_arg;
						target_arg["name"] = "target";
						target_arg["type"] = Variant::OBJECT;
						Dictionary arg1_arg;
						arg1_arg["name"] = "arg1";
						arg1_arg["type"] = Variant::NIL;
						add_user_signal(signal_name, Array({
							target_arg,
							arg1_arg
						}));
						m_signals.push_back(signal_name);
					}
				} break;
				case 2: {
					if (target_object->is_connected(target_signal, callable_mp(this, &GDMLResult::on_defined_signal2))) {
						return;
					} else {
						target_object->connect(target_signal, callable_mp(this, &GDMLResult::on_defined_signal2).bind(signal_name, target_object));
					}

					if (!has_user_signal(signal_name)) {
						Dictionary target_arg;
						target_arg["name"] = "target";
						target_arg["type"] = Variant::OBJECT;
						Dictionary arg1_arg;
						arg1_arg["name"] = "arg1";
						arg1_arg["type"] = Variant::NIL;
						Dictionary arg2_arg;
						arg2_arg["name"] = "arg1";
						arg2_arg["type"] = Variant::NIL;
						add_user_signal(signal_name, Array({
							target_arg,
							arg1_arg,
							arg2_arg
						}));
						m_signals.push_back(signal_name);
					}
				} break;
				}
			}
		}
		
		emit_signal("signals_changed");
	}
}

template<typename... Args>
void GDMLResult::on_defined_signal(Args ...args, String signal_name, Variant target) {
	emit_signal(signal_name, target, ...args);
}

void GDMLResult::on_defined_signal0(String signal_name, Variant target) {
	emit_signal(signal_name, target);
}

void GDMLResult::on_defined_signal1(Variant arg1, String signal_name, Variant target) {
	emit_signal(signal_name, target, arg1);
}

void GDMLResult::on_defined_signal2(Variant arg1, Variant arg2, String signal_name, Variant target) {
	emit_signal(signal_name, target, arg1, arg2);
}

Array GDMLResult::get_nodes() {
	return m_nodes;
}

Array GDMLResult::get_variable_list() {
	return m_variables.keys();
}

Ref<reactive::GDMLAtom> GDMLResult::get_variable(String p_name) {
	if (m_variables.has(p_name)) {
		return m_variables[p_name];
	} else {
		return nullptr;
	}
}

Array GDMLResult::get_signals() {
	return m_signals;
}

void GDMLRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("renderString", "source"), &GDMLRenderer::renderString);
	ClassDB::bind_method(D_METHOD("renderMarkup", "markup_script"), &GDMLRenderer::renderMarkup);
}

GDMLRenderer::GDMLRenderer() :
		m_parser(std::make_unique<GdmlParser>()) {}

Ref<GDMLResult> GDMLRenderer::renderString(const String &source) {
	std::shared_ptr<SourceFileNode> result = m_parser->parse(source);

	print_line(traversal::GdmlPrinterVisitor().visit(*result));

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}

	Ref<GDMLResult> out = Ref<GDMLResult>(memnew(GDMLResult));
	m_visitor = std::make_unique<traversal::TreeRendererVisitor>(out);
	traversal::TreeRendererVisitorResult visitor_result = m_visitor->visit(*result);

	for (Variant node : visitor_result.nodes) {
		out->m_nodes.push_back(node);
	}

	for (std::pair<String, Ref<reactive::GDMLAtom>> pair : visitor_result.variables) {
		out->add_variable(pair.first, pair.second);
	}

	for (std::pair<String, traversal::TreeRendererVisitorContext::SignalTarget> pair : visitor_result.signals) {
		for (std::pair<godot::ObjectID, std::set<StringName>> target_pair : pair.second) {
			for (StringName target_signal : target_pair.second) {
				out->connect_signal(pair.first, target_pair.first, target_signal);
			}
		}
	}

	return out;
}

Ref<GDMLResult> GDMLRenderer::renderMarkup(GdmlScript *script) {
	return renderString(script->get_source_code());
}

}
#include "gdml_renderer.h"

#include "tree_renderer_visitor.h"
#include "diff.h"
#include "tree_patcher.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

namespace gdml {

void GDMLResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_variable_list"), &GDMLResult::get_variable_list);
	ClassDB::bind_method(D_METHOD("get_variable", "name"), &GDMLResult::get_variable);
	
	ClassDB::bind_method(D_METHOD("get_signals"), &GDMLResult::get_signals);
	ClassDB::bind_method(D_METHOD("connect_signal", "name", "callable", "flags"), &GDMLResult::connect_signal, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("attach", "set_owner"), &GDMLResult::attach, DEFVAL(true));
}

GDMLResult::GDMLResult() {}

void GDMLResult::add_variable(String &p_name, Ref<reactive::GDMLAtom> atom) {
	m_variables[p_name] = atom;
}

void GDMLResult::attach(bool p_set_owner) {
	std::function<void(Node *, Node *)> set_owner_recursively = [&set_owner_recursively](Node *node, Node *owner) {
		node->set_owner(owner);
		for (Variant item : node->get_children()) {
			Node *child = Object::cast_to<Node>(item);
			set_owner_recursively(child, owner);
		}
	};

	if (is_first_renderer) {
		for (Variant item : m_root->get_children()) {
			Node *node = cast_to<Node>(item);
			node->queue_free();
		}

		for (Variant item : m_nodes) {
			Node *node = cast_to<Node>(item);
			m_root->add_child(node);
			if (p_set_owner) {
				set_owner_recursively(node, m_root->get_owner() ? m_root->get_owner() : m_root);
			}
		}
		is_first_renderer = false;
	}
}

Node *GDMLResult::get_root() {
	return m_root;
}

Array GDMLResult::get_variable_list() {
	return m_variables.keys();
}

Ref<reactive::GDMLAtom> GDMLResult::get_variable(String p_name) {
	if (m_variables.has(p_name)) {
		return m_variables[p_name];
	} else {
		print_error("Cannot access a variable with name '" + p_name + "'");
		return Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
	}
}

Array GDMLResult::get_signals() {
	godot::Array out;

	for (auto it : m_signals) {
		out.push_back(it.first);
	}

	return out;
}

godot::Error GDMLResult::connect_signal(godot::String p_signal_name, godot::Callable p_callable, uint32_t p_flags) {
	auto it = m_signals.find(p_signal_name);
	if (it == m_signals.end()) {
		return godot::ERR_DOES_NOT_EXIST;
	}

	for (std::pair<const godot::ObjectID, gdml::SignalTargetItem> &target : it->second) {
		Object *obj = godot::ObjectDB::get_instance(target.first);
		if (obj) {
			for (godot::StringName target_signal_name : target.second.signals) {
				godot::Callable bound_callable = p_callable.bindv(target.second.args);
				godot::Error err = obj->connect(target_signal_name, bound_callable, p_flags);
				if (err != godot::OK) {
					return err;
				} else {
					m_connected_signals[p_signal_name] = ConnectedSignal { p_callable, p_flags };
				}
			}
		}
	}

	return godot::OK;
}

void GDMLRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("render_string", "root_node", "source", "variables"), &GDMLRenderer::render_string, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("render_markup", "root_node", "markup_script", "variables"), &GDMLRenderer::render_markup, DEFVAL(Array()));
}

GDMLRenderer::GDMLRenderer() :
		m_parser(std::make_unique<GdmlParser>()) {}

GDMLRenderer::~GDMLRenderer() = default;

Ref<GDMLResult> GDMLRenderer::render_string(Node *p_root, const String &p_source, godot::Array variables) {
	std::shared_ptr<SourceFileNode> result = m_parser->parse(p_source);

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}

	std::vector<godot::Ref<reactive::GDMLAtom>> vars_native;

	for (godot::Variant item : variables) {
		vars_native.push_back(godot::Object::cast_to<reactive::GDMLAtom>(item));
	}

	Ref<GDMLResult> out = Ref<GDMLResult>(memnew(GDMLResult));
	m_visitor = std::make_unique<traversal::TreeRendererVisitor>(out, p_root, vars_native);
	traversal::TreeRendererVisitorResult visitor_result = m_visitor->visit(*result);

	for (std::pair<String, Ref<reactive::GDMLAtom>> pair : visitor_result.variables) {
		out->add_variable(pair.first, pair.second);
	}

	out->m_signals = visitor_result.signals;
	out->m_root = p_root;

	for (Node *node : visitor_result.nodes) {
		out->m_nodes.push_back(node);
	}

	return out;
}

Ref<GDMLResult> GDMLRenderer::render_markup(Node *p_root, GdmlScript *p_script, godot::Array variables) {
	return render_string(p_root, p_script->get_source_code(), variables);
}

}

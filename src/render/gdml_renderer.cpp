#include "gdml_renderer.h"

#include "tree_renderer_visitor.h"
#include "diff.h"
#include "tree_patcher.h"

#include <assert.h>

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
	ClassDB::bind_method(D_METHOD("attach", "set_owner"), &GDMLResult::attach, DEFVAL(true));
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
						arg2_arg["name"] = "arg2";
						arg2_arg["type"] = Variant::NIL;
						add_user_signal(signal_name, Array({
							target_arg,
							arg1_arg,
							arg2_arg
						}));
						m_signals.push_back(signal_name);
					}
				} break;
				case 3: {
					if (target_object->is_connected(target_signal, callable_mp(this, &GDMLResult::on_defined_signal3))) {
						return;
					} else {
						target_object->connect(target_signal, callable_mp(this, &GDMLResult::on_defined_signal3).bind(signal_name, target_object));
					}

					if (!has_user_signal(signal_name)) {
						Dictionary target_arg;
						target_arg["name"] = "target";
						target_arg["type"] = Variant::OBJECT;
						Dictionary arg1_arg;
						arg1_arg["name"] = "arg1";
						arg1_arg["type"] = Variant::NIL;
						Dictionary arg2_arg;
						arg2_arg["name"] = "arg2";
						arg2_arg["type"] = Variant::NIL;
						Dictionary arg3_arg;
						arg3_arg["name"] = "arg3";
						arg3_arg["type"] = Variant::NIL;
						add_user_signal(signal_name, Array({
							target_arg,
							arg1_arg,
							arg2_arg,
							arg3_arg,
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
	} else {
		gdml::Differ differ;
		std::vector<Node *> old_nodes;
		std::vector<Node *> new_nodes;
		for (Variant item : m_root->get_children()) {
			Node *node = cast_to<Node>(item);
			old_nodes.push_back(node);
		}
		for (Variant item : m_nodes) {
			Node *node = cast_to<Node>(item);
			new_nodes.push_back(node);
		}
		std::vector<gdml::DiffItem> diff;
		differ.diff(old_nodes, new_nodes, m_root, diff);
		for (gdml::DiffItem &item : diff) {
			if (item.type == gdml::DiffType::DELETE) {
				print_line("DELETE: ", item.node);
			} else if (item.type == gdml::DiffType::INSERT) {
				print_line("INSERT: ", item.node);
			} else if (item.type == gdml::DiffType::MOVE) {
				print_line("MOVE: ", item.node);
			} else if (item.type == gdml::DiffType::UPDATE) {
				print_line("UPDATE: ", item.node);
			}
		}
		gdml::TreePatcher patcher;
		patcher.patch(m_root, diff);
		for (Variant item : m_root->get_children()) {
			Node *node = cast_to<Node>(item);
			if (p_set_owner) {
				set_owner_recursively(node, m_root->get_owner() ? m_root->get_owner() : m_root);
			}
		}
	}
}

Node *GDMLResult::get_root() {
	return m_root;
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

void GDMLResult::on_defined_signal3(Variant arg1, Variant arg2, Variant arg3, String signal_name, Variant target) {
	emit_signal(signal_name, target, arg1, arg2, arg3);
}

Array GDMLResult::get_variable_list() {
	return m_variables.keys();
}

Ref<reactive::GDMLAtom> GDMLResult::get_variable(String p_name) {
	if (m_variables.has(p_name)) {
		return m_variables[p_name];
	} else {
		print_error("Cannot access variable with name '" + p_name + "'");
		return Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
	}
}

Array GDMLResult::get_signals() {
	return m_signals;
}

void GDMLRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("render_string", "root_node", "source"), &GDMLRenderer::render_string);
	ClassDB::bind_method(D_METHOD("render_markup", "root_node", "markup_script"), &GDMLRenderer::render_markup);
}

GDMLRenderer::GDMLRenderer() :
		m_parser(std::make_unique<GdmlParser>()) {}

Ref<GDMLResult> GDMLRenderer::render_string(Node *p_root, const String &p_source) {
	std::shared_ptr<SourceFileNode> result = m_parser->parse(p_source);

	print_line(traversal::GdmlPrinterVisitor().visit(*result));

	for (GdmlParseError &err : m_parser->getParseErrors()) {
		print_error(err.toString());
	}

	Ref<GDMLResult> out = Ref<GDMLResult>(memnew(GDMLResult));
	m_visitor = std::make_unique<traversal::TreeRendererVisitor>(out);
	traversal::TreeRendererVisitorResult &visitor_result = m_visitor->visit(*result);

	for (std::pair<String, Ref<reactive::GDMLAtom>> pair : visitor_result.variables) {
		out->add_variable(pair.first, pair.second);
	}

	out->m_root = p_root;
	
	for (Node *node : visitor_result.nodes) {
		out->m_nodes.push_back(node);
	}

	return out;
}

Ref<GDMLResult> GDMLRenderer::render_markup(Node *p_root, GdmlScript *p_script) {
	return render_string(p_root, p_script->get_source_code());
}

}
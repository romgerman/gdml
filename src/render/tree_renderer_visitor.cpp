#include "tree_renderer_visitor.h"
#include "gdml_renderer.h"
#include "gdml_decorator.h"

#include <cassert>
#include <functional>

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

using namespace godot;
using namespace parser;

namespace traversal {

#pragma region Renderer

TreeRendererVisitor::TreeRendererVisitor(Ref<gdml::GDMLResult> result, godot::Node *p_root) {
	m_result = result;
	m_root_node = p_root;
}

TreeRendererVisitorResult TreeRendererVisitor::visit(const GdmlNode &root_node) {
	m_context = std::make_unique<TreeRendererVisitorContext>();
	m_context->get_scope()->set_parent_node(m_root_node);
	m_prop_helper = std::make_unique<PropertyHelper>(m_context.get(), &m_decorator_cache);

	if (root_node.type == GDMLNodeType::SOURCE_FILE) {
		TreeRendererVisitorResult result;
		ScopeGuard guard(*m_context);
		visitSourceFile((SourceFileNode &)root_node, result);
		m_context->flush_deferred_properties();
		result.variables = m_context->m_variables;
		result.signals = m_context->m_signals;
		return result;
	}

	m_context->push_error("Cannot render tree. The tree is malformed", root_node);
	return TreeRendererVisitorResult();
}

void TreeRendererVisitor::visitSourceFile(const SourceFileNode &source_node, TreeRendererVisitorResult &result) {
	for (const std::unique_ptr<ImportStatementNode> &statement : source_node.imports) {
		visitImportStatement(*statement);
	}

	for (const std::unique_ptr<DefinitionNode> &definition : source_node.statements) {
		switch (definition->statement->type)
		{
		case GDMLNodeType::NODE_DEFINITION: {
			const NodeDefinitionNode *node_def = static_cast<const NodeDefinitionNode *>(definition->statement.get());

			ScopeGuard guard(*m_context);
			Variant result_node = visitNodeDefinition(*node_def);
			if (result_node) {
				result.nodes.push_back(result_node);
			}
		} break;
		case GDMLNodeType::VARIABLE_DEFINITION: {
			Variant value;
			VariableDefinitionNode *var = static_cast<VariableDefinitionNode *>(definition->statement.get());

			if (var->value) {
				if (m_prop_helper->resolve(value, *var->value, value)) {
					m_context->set_reactive_variable(var->name->name, value);
				} else {
					m_context->push_error("Couldn't resolve variable expression", *var);
				}
			} else {
				m_context->set_reactive_variable(var->name->name, Variant());
			}
		} break;
		case GDMLNodeType::COMPONENT_DEFINITION: {
			ComponentDefinitionNode *component = static_cast<ComponentDefinitionNode *>(definition->statement.get());
			auto component_item = TreeRendererVisitorContext::Component();
			component_item.node = static_cast<NodeDefinitionNode *>(component->value.get());
			
			for (std::unique_ptr<godot::IdentifierNode> &p : component->params) {
				component_item.params.push_back(p->name);
			}

			m_context->m_components[component->name->name] = component_item;
		} break;
		case GDMLNodeType::FOR_STATEMENT: {
			ForStatementNode *statement = static_cast<ForStatementNode *>(definition->statement.get());
			m_context->push_scope();
			visitForStatement(statement, result.nodes);
			m_context->pop_scope();
		} break;
		case GDMLNodeType::IF_STATEMENT: {
			IfStatementNode *statement = static_cast<IfStatementNode *>(definition->statement.get());
			m_context->push_scope();
			visitIfStatement(statement, result.nodes);
			m_context->pop_scope();
		} break;
		case GDMLNodeType::SIGNAL_DEFINITION: {
			SignalDefinitionNode *def = static_cast<SignalDefinitionNode *>(definition->statement.get());
			visitSignalDefinition(*def);
		} break;
		}
	}
}

Variant TreeRendererVisitor::visitNodeDefinition(const NodeDefinitionNode &node) {
	const String &node_name = node.name->name;
	Variant root_variant;

	if (m_context->m_imports.find(node_name) != m_context->m_imports.end()) {
		const String &import_path = m_context->m_imports[node_name];

		if (import_path.ends_with(".tscn") || import_path.ends_with(".scn")) {
			Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(import_path, "PackedScene");
	
			if (scene.is_null()) {
				m_context->push_error("Cannot load import path " + import_path, node);
				return nullptr;
			}
	
			Node *base_object = scene->instantiate();
			assert(base_object);
			root_variant = base_object;
		} else if (import_path.ends_with(".gd")) {
			Ref<Script> script = ResourceLoader::get_singleton()->load(import_path, "Script");
			if (script.is_null()) {
				m_context->push_error("Cannot load script associated with type '" + node_name + "' by path " + import_path, node);
				return nullptr;
			}
			root_variant = ClassDB::instantiate(script->get_instance_base_type());
			Object *base_object = root_variant.get_validated_object();
			assert(base_object);
			base_object->set_script(script);
		} else if (import_path.ends_with(".gdml")) {
			Ref<Script> script = ResourceLoader::get_singleton()->load(import_path, "Script");
			if (script.is_null()) {
				m_context->push_error("Cannot load markup associated with type '" + node_name + "' by path " + import_path, node);
				return nullptr;
			}
			GdmlParser parser;
			const SourceFileNode &source_file = m_document.load(import_path, script->get_source_code());
			std::unique_ptr<TreeRendererVisitorContext::Component> component = std::make_unique<TreeRendererVisitorContext::Component>();

			for (const std::unique_ptr<DefinitionNode> &def : source_file.statements) {
				if (def->statement->type == GDMLNodeType::COMPONENT_DEFINITION) {
					ComponentDefinitionNode *cmp = static_cast<ComponentDefinitionNode *>(def->statement.get());
					if (cmp->name->name == node_name) {
						component->node = cmp->value.get();
						for (std::unique_ptr<godot::IdentifierNode> &p : cmp->params) {
							component->params.push_back(p->name);
						}
						break;
					}
				}
			}

			if (component->node) {
				std::shared_ptr<Scope> current_scope = m_context->m_current_scope;
				std::shared_ptr<Scope> component_scope = std::make_shared<Scope>();
				component_scope->set_parent_node(current_scope->get_parent_node());

				for (String &param_name : component->params) {
					for (const std::unique_ptr<godot::PropertyDefinitionNode> &prop : node.properties) {
						if (prop->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
							IdentifierNode &prop_name = static_cast<IdentifierNode &>(*prop->name);
							if (prop_name.name == param_name) {
								Variant out;
								if (m_prop_helper->resolve(root_variant, *prop->value, out)) {
									component_scope->set_variable(param_name, out);
								} else {
									m_context->push_error("Cannot resolve expression while rendering a component", *prop);
								}
							}
						}
					}
				}

				m_context->m_current_scope = component_scope;
				root_variant = visitNodeDefinition(*component->node);
				m_context->m_current_scope = current_scope;
			} else {
				m_context->push_error("Cannot find an imported component with name '" + node_name + "' [" + import_path + "]", node);
				return nullptr;
			}
		}
	} else if (m_context->m_components.find(node_name) != m_context->m_components.end()) {
		TreeRendererVisitorContext::Component &cmp = m_context->m_components[node_name];
		std::shared_ptr<Scope> current_scope = m_context->m_current_scope;
		std::shared_ptr<Scope> component_scope = std::make_shared<Scope>();

		for (String &param_name : cmp.params) {
			for (const std::unique_ptr<godot::PropertyDefinitionNode> &prop : node.properties) {
				if (prop->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode &prop_name = static_cast<IdentifierNode &>(*prop->name);
					if (prop_name.name == param_name) {
						Variant out;
						if (m_prop_helper->resolve(root_variant, *prop->value, out)) {
							component_scope->set_variable(param_name, out);
						} else {
							m_context->push_error("Cannot resolve expression while rendering a component", *prop);
						}
					}
				}
			}
		}

		m_context->m_current_scope = component_scope;
		root_variant = visitNodeDefinition(*cmp.node);
		m_context->m_current_scope = current_scope;
	} else {
		if (!ClassDB::class_exists(node_name)) {
			Dictionary class_info;
	
			for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
				if (String(ci.get("class", "")) == node_name) {
					class_info = ci;
					break;
				}
			}
	
			if (class_info.is_empty()) {
				m_context->push_error("Class with name '" + node_name + "' is not found", node);
				return nullptr;
			}
	
			String path = class_info.get("path", "");
			Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
			if (script.is_null()) {
				m_context->push_error("Cannot load script associated with type '" + node_name + "' by path " + path, node);
				return nullptr;
			}
			root_variant = ClassDB::instantiate(script->get_instance_base_type());
			Object *base_object = root_variant.get_validated_object();
			assert(base_object);
			base_object->set_script(script);
		} else {
			if (!ClassDB::can_instantiate(node_name)) {
				m_context->push_error("Cannot construct node of type '" + node_name + "'", node);
				return nullptr;
			}
	
			root_variant = ClassDB::instantiate(node_name);
		}
	}

	bool is_node = false;
	Node *obj_as_node;
	if (root_variant.get_type() == Variant::Type::OBJECT) {
		obj_as_node = Object::cast_to<Node>(root_variant);
		if (obj_as_node) {
			is_node = true;
		} else {
			m_context->push_error("Object is not a node", node);
			Object *obj = Object::cast_to<Object>(root_variant);
			if (obj && !Object::cast_to<RefCounted>(obj)) {
				memdelete(obj);
			}
			return nullptr;
		}
	} else {
		m_context->push_error("Variant type is not an object", node);
		return nullptr;
	}

	m_context->get_scope()->set_parent_node(obj_as_node);

	for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
		if (property->is_theme_override) {
			Control *control = Object::cast_to<Control>(root_variant);
			if (control) {
				if (property->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
					setControlThemeOverride(root_variant, root_variant, identifier->name, *property->value.get());
				}
			} else {
				m_context->push_error("Trying to set theme override on a non-Control node", *property);
			}
		} else {
			if (property->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				trySetVariantProperty(root_variant, root_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GDMLNodeType::PROPERTY_PATH) {
				PropertyPathNode *path = static_cast<PropertyPathNode *>(property->name.get());
				m_prop_helper->set_deep_property(root_variant, root_variant, *path, *property->value.get());
			} else if (property->name->type == GDMLNodeType::SIGNAL_EXPRESSION) {
				const SignalExpressionNode *signal_expression = static_cast<SignalExpressionNode *>(property->name.get());
				if (signal_expression->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					const String signal_name = static_cast<IdentifierNode *>(signal_expression->name.get())->name;
					if (property->value->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<IdentifierNode *>(property->value.get())->name);
					} else if (property->value->type == GDMLNodeType::STRING_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<StringNode *>(property->value.get())->value);
					} else {
						m_context->push_error("Cannot connect signal. Callable name should be an identifier or a string", *property);
					}
				} else {
					m_context->push_error("Cannot connect signal. Wrong name expression type", *property);
				}
			}
		}
	}

	for (const std::unique_ptr<VariableDefinitionNode> &var : node.variables) {
		Variant value;
		if (m_prop_helper->resolve(value, *var, value)) {
			m_context->get_scope()->set_variable(var->name->name, value);
		}
	}

	for (const std::unique_ptr<FunctionCallExpressionNode> &dec_fn : node.decorators) {
		if (dec_fn->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
			IdentifierNode *dec_fn_name = static_cast<IdentifierNode *>(dec_fn->name.get());
			Ref<gdml::GDMLDecorator> decorator = m_decorator_cache.store(dec_fn_name->name);

			if (decorator.is_null()) {
				m_context->push_error("Decorator script for '" + dec_fn_name->name + "' not found", node);
				continue;
			}

			Array call_args;
			call_args.push_back(obj_as_node);
			
			Array args;
			for (std::unique_ptr<ExpressionNode> &arg_node : dec_fn->arguments) {
				Variant arg_value;
				if (m_prop_helper->resolve(root_variant, *arg_node, arg_value)) {
					args.push_back(arg_value);
				} else {
					m_context->push_error("Cannot resolve decorator argument", *arg_node);
				}
			}

			call_args.push_back(args);

			if (decorator->has_method(StringName("modify"))) {
				decorator->callv(StringName("modify"), call_args);
			}
		}
	}

	for (const std::unique_ptr<StatementNode> &statement : node.statements) {
		if (statement->type == GDMLNodeType::NODE_DEFINITION) {
			const NodeDefinitionNode *child = static_cast<const NodeDefinitionNode *>(statement.get());
			m_context->push_scope();
			Variant child_variant = visitNodeDefinition(*child);
			m_context->pop_scope();

			if (child_variant.get_type() != Variant::Type::NIL) {
				Object *child_obj = child_variant.get_validated_object();
				assert(child_obj);
				Node *child_node = Object::cast_to<Node>(child_obj);
				assert(child_node);
				obj_as_node->add_child(child_node);
			}
		} else if (statement->type == GDMLNodeType::FOR_STATEMENT) {
			ForStatementNode *for_node = static_cast<ForStatementNode *>(statement.get());
			std::vector<Variant> nodes;
			m_context->push_scope();
			visitForStatement(for_node, nodes);
			m_context->pop_scope();

			for (Variant &item : nodes) {
				obj_as_node->add_child(Object::cast_to<Node>(item));
			}
		} else if (statement->type == GDMLNodeType::IF_STATEMENT) {
			IfStatementNode *if_node = static_cast<IfStatementNode *>(statement.get());
			std::vector<Variant> nodes;
			m_context->push_scope();
			visitIfStatement(if_node, nodes);
			m_context->pop_scope();

			for (Variant &item : nodes) {
				obj_as_node->add_child(Object::cast_to<Node>(item));
			}
		}
	}

	return root_variant;
}

void TreeRendererVisitor::visitForStatement(ForStatementNode *statement, std::vector<Variant> &out_nodes) {
	std::shared_ptr<FragmentContext> fragment = std::make_shared<FragmentContext>();
	fragment->rendered_nodes = std::make_shared<std::vector<Variant>>();
	fragment->scope = m_context->m_current_scope;
	fragment->parent_id = (ObjectID)m_context->get_scope()->get_parent_node()->get_instance_id();
	fragment->statement = statement;

	if (statement->range->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &range_expr = static_cast<IdentifierNode &>(*statement->range);
		if (m_context->has_reactive_variable(range_expr.name)) {
			m_context->set_active_variable(range_expr.name);
		}
	}
	
	renderForStatement(statement, fragment->rendered_nodes, out_nodes);

	std::function<void()> render_callback = [fragment, this]() {
		Object *parent_obj = ObjectDB::get_instance(fragment->parent_id);
		if (!parent_obj) {
			print_line("Trying to render for-statement with non-existent or invalid parent node");
			return;
		}

		Node *parent = Object::cast_to<Node>(parent_obj);

		for (Variant item : *fragment->rendered_nodes) {
			Node *node = Object::cast_to<Node>(item);
			if (node) {
				node->queue_free();
			}
		}

		fragment->rendered_nodes->clear();

		std::vector<Variant> cb_nodes;
		std::shared_ptr<Scope> prev_scope = m_context->m_current_scope;
		m_context->m_current_scope = fragment->scope;
		renderForStatement(static_cast<ForStatementNode*>(fragment->statement), fragment->rendered_nodes, cb_nodes);
		m_context->m_current_scope = prev_scope;

		for (auto result_node : cb_nodes) {
			Node *node = Object::cast_to<Node>(result_node);
			if (node) {
				parent->add_child(node);
				node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
				fragment->rendered_nodes->push_back(result_node);
			}
		}
		
		m_context->flush_deferred_properties();
		updateSignals();
	};

	for (Ref<reactive::GDMLAtom> atom : m_context->get_active_variables()) {
		atom->add_callback(render_callback);
	}

	if (statement->range->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &range_expr = static_cast<IdentifierNode &>(*statement->range);
		if (m_context->has_reactive_variable(range_expr.name)) {
			m_context->reset_active_variable(range_expr.name);
		}
	}
}

void TreeRendererVisitor::renderForStatement(ForStatementNode *statement, const std::shared_ptr<std::vector<Variant>> &nodes, std::vector<Variant> &out_nodes) {
	String iter_name;

	if (statement->iter_name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		iter_name = static_cast<IdentifierNode &>(*statement->iter_name).name;
	} else {
		m_context->push_error("Cannot use non-identifier expressions in for-statement iterator", *statement);
		return;
	}

	String iter_name2;
	bool has_iter2 = false;

	if (statement->iter_name2) {
		if (statement->iter_name2->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
			iter_name2 = static_cast<IdentifierNode &>(*statement->iter_name2).name;
			has_iter2 = true;
		} else {
			m_context->push_error("Cannot use non-identifier expressions in for-statement iterator", *statement);
			return;
		}
	}

	Variant range_variant;
	if (m_prop_helper->resolve(range_variant, *statement->range, range_variant)) {
		if (range_variant.get_type() == Variant::ARRAY) {
			Array arr = range_variant;
			for (int64_t i = 0; i < arr.size(); i++) {
				Variant item = arr[i];
				if (has_iter2) {
					m_context->get_scope()->set_variable(iter_name, i);
					m_context->get_scope()->set_variable(iter_name2, item);
				} else {
					m_context->get_scope()->set_variable(iter_name, item);
				}
				for (std::unique_ptr<NodeDefinitionNode> &item : statement->statements) {
					m_context->push_active_context();
					Variant result_node = visitNodeDefinition(*item);
					m_context->pop_active_context();
					if (result_node) {
						out_nodes.push_back(result_node);
						nodes->push_back(result_node);
					}
				}
			}
		} else if (range_variant.get_type() == Variant::DICTIONARY) {
			Dictionary dict = range_variant;
			for (Variant key : dict.keys()) {
				Variant value = dict[key];
				if (has_iter2) {
					m_context->get_scope()->set_variable(iter_name, key);
					m_context->get_scope()->set_variable(iter_name2, value);
				} else {
					m_context->get_scope()->set_variable(iter_name, key);
				}
				for (std::unique_ptr<NodeDefinitionNode> &item : statement->statements) {
					m_context->push_active_context();
					Variant result_node = visitNodeDefinition(*item);
					m_context->pop_active_context();
					if (result_node) {
						out_nodes.push_back(result_node);
						nodes->push_back(result_node);
					}
				}
			}
		} else {
			for (int32_t i = 0; i < (int32_t)range_variant; i++) {
				m_context->get_scope()->set_variable(iter_name, i);
				for (std::unique_ptr<NodeDefinitionNode> &item : statement->statements) {
					m_context->push_active_context();
					Variant result_node = visitNodeDefinition(*item);
					m_context->pop_active_context();
					if (result_node) {
						out_nodes.push_back(result_node);
						nodes->push_back(result_node);
					}
				}
			}
		}
	} else {
		m_context->push_error("Cannot resolve range expression of for-statement", *statement->range);
	}
}

void TreeRendererVisitor::visitIfStatement(IfStatementNode *statement, std::vector<Variant> &out_nodes) {
	std::shared_ptr<FragmentContext> fragment = std::make_shared<FragmentContext>();
	fragment->rendered_nodes = std::make_shared<std::vector<Variant>>();
	fragment->scope = m_context->m_current_scope;
	fragment->parent_id = (ObjectID)m_context->get_scope()->get_parent_node()->get_instance_id();
	fragment->statement = statement;

	if (statement->expression->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &range_expr = static_cast<IdentifierNode &>(*statement->expression);
		if (m_context->has_reactive_variable(range_expr.name)) {
			m_context->set_active_variable(range_expr.name);
		}
	}
	
	renderIfStatement(statement, fragment->rendered_nodes, out_nodes);

	std::function<void()> render_callback = [fragment, this]() {
		Object *parent_obj = ObjectDB::get_instance(fragment->parent_id);
		if (!parent_obj) {
			print_line("Trying to render if-statement with non-existent or invalid parent node");
			return;
		}

		Node *parent = Object::cast_to<Node>(parent_obj);

		for (Variant item : *fragment->rendered_nodes) {
			Node *node = Object::cast_to<Node>(item);
			if (node) {
				node->queue_free();
			}
		}

		fragment->rendered_nodes->clear();

		std::vector<Variant> cb_nodes;
		std::shared_ptr<Scope> prev_scope = m_context->m_current_scope;
		m_context->m_current_scope = fragment->scope;
		renderIfStatement(static_cast<IfStatementNode*>(fragment->statement), fragment->rendered_nodes, cb_nodes);
		m_context->m_current_scope = prev_scope;

		for (auto result_node : cb_nodes) {
			Node *node = Object::cast_to<Node>(result_node);
			if (node) {
				parent->add_child(node);
				node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
				fragment->rendered_nodes->push_back(result_node);
			}
		}
		
		m_context->flush_deferred_properties();
		updateSignals();
	};

	for (Ref<reactive::GDMLAtom> atom : m_context->get_active_variables()) {
		atom->add_callback(render_callback);
	}

	if (statement->expression->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &range_expr = static_cast<IdentifierNode &>(*statement->expression);
		if (m_context->has_reactive_variable(range_expr.name)) {
			m_context->reset_active_variable(range_expr.name);
		}
	}
}

void TreeRendererVisitor::renderIfStatement(IfStatementNode *statement, const std::shared_ptr<std::vector<Variant>> &nodes, std::vector<Variant> &out_nodes) {
	Variant truthy_variant;
	if (m_prop_helper->resolve(truthy_variant, *statement->expression, truthy_variant)) {
		if (truthy_variant) {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement->if_statements) {
				m_context->push_active_context();
				Variant result_node = visitNodeDefinition(*item);
				m_context->pop_active_context();
				if (result_node) {
					out_nodes.push_back(result_node);
					nodes->push_back(result_node);
				}
			}
		} else {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement->else_statements) {
				m_context->push_active_context();
				Variant result_node = visitNodeDefinition(*item);
				m_context->pop_active_context();
				if (result_node) {
					out_nodes.push_back(result_node);
					nodes->push_back(result_node);
				}
			}
		}
	} else {
		m_context->push_error("Cannot resolve if-statement expression", *statement->expression);
	}
}

void TreeRendererVisitor::visitSignalDefinition(SignalDefinitionNode &definition) {
	m_context->m_signals[definition.name->name] = TreeRendererVisitorContext::SignalTarget();
}

void TreeRendererVisitor::visitImportStatement(godot::ImportStatementNode &statement) {
	m_context->m_imports[statement.name->name] = statement.from->value;
}

bool TreeRendererVisitor::trySetVariantProperty(Variant &root_variant, Variant &variant, String &prop_key, ExpressionNode &value_node) {
	if (PropertyHelper::references_self(value_node)) {
		std::function<void()> deferred_set = [this, root_variant, variant, prop_key, &value_node]() {
			m_prop_helper->set_property(const_cast<Variant &>(root_variant), const_cast<Variant &>(variant), const_cast<String &>(prop_key), value_node);
		};

		m_context->m_deferred_properties.push_back(deferred_set);
		return true;
	} else{
		return m_prop_helper->set_property(root_variant, variant, prop_key, value_node);
	}
}

void TreeRendererVisitor::setControlThemeOverride(Variant &root_variant, Variant &variant, String &prop_key, const ExpressionNode &value_node) {
	Control *control = Object::cast_to<Control>(variant);

	if (value_node.type == GDMLNodeType::NUMBER_EXPRESSION) {
		NumberNode node = (NumberNode &)value_node;

		if (control->has_theme_constant(prop_key)) {
			control->add_theme_constant_override(prop_key, node.getInteger());
		} else if (control->has_theme_font_size(prop_key)) {
			control->add_theme_font_size_override(prop_key, node.getInteger());
		}
	} else if (value_node.type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &node = (IdentifierNode &)value_node;

		if (m_context->has_reactive_variable(node.name)) {
			subscribeToVariable(node.name, prop_key, variant);
		}

		Variant out_value;
		if (m_prop_helper->get_property(root_variant, node.name, out_value)) {
			if (out_value.get_type() == Variant::INT || out_value.get_type() == Variant::FLOAT) {
				if (control->has_theme_constant(prop_key)) {
					control->add_theme_constant_override(prop_key, out_value);
				} else if (control->has_theme_font_size(prop_key)) {
					control->add_theme_font_size_override(prop_key, out_value);
				}
			} else if (out_value.get_type() == Variant::COLOR) {
				if (control->has_theme_color(prop_key)) {
					control->add_theme_color_override(prop_key, out_value);
				}
			} else {
				if (control->has_theme_icon(prop_key)) {
					control->add_theme_icon_override(prop_key, out_value);
				} else if (control->has_theme_stylebox(prop_key)) {
					control->add_theme_stylebox_override(prop_key, out_value);
				} else if (control->has_theme_font(prop_key)) {
					control->add_theme_font_override(prop_key, out_value);
				}
			}
		}
	} else if (value_node.type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
		MemberExpressionNode &node = (MemberExpressionNode &)value_node;
		Variant out_value;
		if (m_prop_helper->get_deep_property(root_variant, node, out_value)) {
			if (out_value.get_type() == Variant::INT || out_value.get_type() == Variant::FLOAT) {
				if (control->has_theme_constant(prop_key)) {
					control->add_theme_constant_override(prop_key, out_value);
				} else if (control->has_theme_font_size(prop_key)) {
					control->add_theme_font_size_override(prop_key, out_value);
				}
			} else if (out_value.get_type() == Variant::COLOR) {
				if (control->has_theme_color(prop_key)) {
					control->add_theme_color_override(prop_key, out_value);
				}
			} else {
				if (control->has_theme_icon(prop_key)) {
					control->add_theme_icon_override(prop_key, out_value);
				} else if (control->has_theme_stylebox(prop_key)) {
					control->add_theme_stylebox_override(prop_key, out_value);
				} else if (control->has_theme_font(prop_key)) {
					control->add_theme_font_override(prop_key, out_value);
				}
			}
		} else {
			m_context->push_error("Cannot resolve expression", node);
		}
	} else if (value_node.type == GDMLNodeType::RESOURCE_DEFINITION) {
		const ResourceDefinitionNode &node = (ResourceDefinitionNode &)value_node;
		Variant resource_variant;

		if (VariantRegistry::is_builtin(node.name->name)) {
			if (!VariantRegistry::construct(node.name->name, resource_variant)) {
				print_error("Error constructing variant of type " + node.name->name);
				return;
			}
		} else {
			if (ClassDB::class_exists(node.name->name)) {
				resource_variant = ClassDB::instantiate(node.name->name);
			} else {
				print_error("Error instantiating class of type " + node.name->name);
				return;
			}
		}

		for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
			if (property->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				m_prop_helper->set_property(root_variant, resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GDMLNodeType::PROPERTY_PATH) {
				PropertyPathNode *path = static_cast<PropertyPathNode *>(property->name.get());
				m_prop_helper->set_deep_property(root_variant, resource_variant, *path, *property->value.get());
			}
		}

		if (control->has_theme_icon(prop_key)) {
			control->add_theme_icon_override(prop_key, resource_variant);
		} else if (control->has_theme_stylebox(prop_key)) {
			control->add_theme_stylebox_override(prop_key, resource_variant);
		} else if (control->has_theme_font(prop_key)) {
			control->add_theme_font_override(prop_key, resource_variant);
		}
	} else if (value_node.type == GDMLNodeType::FUNCTION_CALL) {
		const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)value_node;

		if (node.name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
			IdentifierNode *name_node = static_cast<IdentifierNode *>(node.name.get());

			if (m_context->has_reactive_variable(name_node->name)) {
				Ref<reactive::GDMLAtom> atom = m_context->get_reactive_variable(name_node->name);
				Variant value = atom->get_value();

				if (value.get_type() == Variant::Type::CALLABLE) {
					Callable callable = value;
					Array args;

					for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
						Variant arg_value;
						if (m_prop_helper->resolve(root_variant, *expr.get(), arg_value)) {
							args.push_back(arg_value);
						} else {
							print_error("Unsupported argument of type " + String::num_int64((int)expr->type) + " in function \"" + name_node->name + "\"");
						}
					}

					const Variant result = callable.callv(args);

					if (result.get_type() == Variant::INT || result.get_type() == Variant::FLOAT) {
						if (control->has_theme_constant(prop_key)) {
							control->add_theme_constant_override(prop_key, result);
						} else if (control->has_theme_font_size(prop_key)) {
							control->add_theme_font_size_override(prop_key, result);
						}
					} else if (result.get_type() == Variant::COLOR) {
						if (control->has_theme_color(prop_key)) {
							control->add_theme_color_override(prop_key, result);
						}
					} else {
						if (control->has_theme_icon(prop_key)) {
							control->add_theme_icon_override(prop_key, result);
						} else if (control->has_theme_stylebox(prop_key)) {
							control->add_theme_stylebox_override(prop_key, result);
						} else if (control->has_theme_font(prop_key)) {
							control->add_theme_font_override(prop_key, result);
						}
					}
				} else {
					print_error("Cannot call non-Callable");
				}
			}
		} else {
			print_error("Other types of expressions are not implemented for a function call");
		}
	}
}

bool TreeRendererVisitor::connectSignal(Variant &root_variant, Variant &variant, const String &signal_name, String &callable_name) {
	Object *obj = variant.get_validated_object();
	
	if (m_context->has_global_signal(callable_name)) {
		m_result->connect_signal(callable_name, (ObjectID)obj->get_instance_id(), signal_name);
		return true;
	}

	if (obj) {
		const Variant name_or_callable = m_context->get_scope()->get_variable(callable_name);
		if (name_or_callable) {
			if (name_or_callable.get_type() == Variant::STRING) {
				obj->connect(signal_name, Callable(root_variant, name_or_callable));
				return true;
			} else if (name_or_callable.get_type() == Variant::CALLABLE) {
				obj->connect(signal_name, name_or_callable);
				return true;
			} else {
				print_error("Trying to connect signal with \"" + name_or_callable.get_type_name(name_or_callable.get_type()) + "\"");
			}
		}
	} else {
		print_error("Trying to connect signal for non-Object type");
	}

	return false;
}

void TreeRendererVisitor::subscribeToVariable(String &p_variable, String &p_prop, Variant &variant) {
	Ref<reactive::GDMLAtom> atom = m_context->get_reactive_variable(p_variable);
	atom->add_dependency(variant, p_prop);
}

void TreeRendererVisitor::updateSignals() {
	for (std::pair<String, traversal::TreeRendererVisitorContext::SignalTarget> pair : m_context->m_signals) {
		std::vector<ObjectID> to_remove;

		for (std::pair<ObjectID, std::set<StringName>> target_pair : pair.second) {
			Object *target_obj = ObjectDB::get_instance(target_pair.first);

			if (target_obj) {
				for (StringName target_signal : target_pair.second) {
					m_result->connect_signal(pair.first, target_pair.first, target_signal);
				}
			} else {
				to_remove.push_back(target_pair.first);
			}
		}

		for (ObjectID id : to_remove) {
			m_context->m_signals[pair.first].erase(id);
		}
	}
}

#pragma endregion

} //namespace traversal

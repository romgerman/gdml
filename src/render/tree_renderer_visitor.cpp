#include "tree_renderer_visitor.h"
#include "gdml_renderer.h"
#include "gdml_decorator.h"
#include "fragment.hpp"
#include "diff.h"
#include "tree_patcher.h"

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

TreeRendererVisitor::TreeRendererVisitor(godot::Ref<gdml::GDMLResult> p_result, godot::Node *p_parent) {
	m_result = p_result;
	m_parent = p_parent;
}

TreeRendererVisitorResult TreeRendererVisitor::visit(const GdmlNode &root_node) {
	m_context = std::make_unique<TreeRendererVisitorContext>();
	m_prop_helper = std::make_unique<PropertyHelper>(m_context.get(), &m_decorator_cache);
	m_context->get_scope()->set_parent_node(m_parent);

	if (root_node.type == GDMLNodeType::SOURCE_FILE) {
		TreeRendererVisitorResult result;
		ScopeGuard guard(*m_context);
		visitSourceFile((SourceFileNode &)root_node, result);
		m_context->flush_deferred_actions();
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
			Node *result_node = visitNodeDefinition(*node_def);
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
		case GDMLNodeType::FOR_STATEMENT:
		case GDMLNodeType::IF_STATEMENT: {
			ScopeGuard guard(*m_context);
			visitStatement(definition->statement.get(), result.nodes);
		} break;
		case GDMLNodeType::SIGNAL_DEFINITION: {
			SignalDefinitionNode *def = static_cast<SignalDefinitionNode *>(definition->statement.get());
			visitSignalDefinition(*def);
		} break;
		}
	}
}

Node *TreeRendererVisitor::visitNodeDefinition(const NodeDefinitionNode &node) {
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
			
			Array args;
			for (std::unique_ptr<ExpressionNode> &arg_node : dec_fn->arguments) {
				Variant arg_value;
				if (m_prop_helper->resolve(root_variant, *arg_node, arg_value)) {
					args.push_back(arg_value);
				} else {
					m_context->push_error("Cannot resolve decorator argument", *arg_node);
				}
			}

			m_context->m_deferred_actions.push_back(std::make_unique<DeferredDecoratorCall>(
				args,
				(ObjectID)obj_as_node->get_instance_id(),
				decorator,
				m_result
			));
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
		} else if (statement->type == GDMLNodeType::FOR_STATEMENT || statement->type == GDMLNodeType::IF_STATEMENT) {
			std::vector<Node *> nodes;
			m_context->push_scope();
			visitStatement(statement.get(), nodes);
			m_context->pop_scope();

			for (Node *node : nodes) {
				obj_as_node->add_child(node);
			}
		}
	}

	return obj_as_node;
}

void TreeRendererVisitor::visitStatement(godot::StatementNode *statement, std::vector<godot::Node *> &out_nodes) {
	std::shared_ptr<FragmentContext> fragment = std::make_shared<FragmentContext>();
	fragment->rendered_nodes = std::make_shared<std::vector<Node *>>();
	fragment->scope = m_context->m_current_scope;
	fragment->parent_id = (ObjectID)m_context->get_scope()->get_parent_node()->get_instance_id();
	fragment->statement = statement;

	// Store the fragment as a child of the parent fragment if any
	if (!m_fragments_stack.empty()) {
		m_fragments_stack.back()->children.push_back(fragment);
	} else {
		m_result->m_owned_fragments.push_back(fragment);
	}
	
	// First time render
	m_fragments_stack.push_back(fragment);
	if (statement->type == GDMLNodeType::FOR_STATEMENT) {
		renderForStatement(static_cast<ForStatementNode *>(statement), fragment->rendered_nodes);
	} else if (statement->type == GDMLNodeType::IF_STATEMENT) {
		renderIfStatement(static_cast<IfStatementNode *>(statement), fragment->rendered_nodes);
	}
	out_nodes = *fragment->rendered_nodes;
	m_fragments_stack.pop_back();
	
	Node *anchor = memnew(Node);
	anchor->set_name(StringName("GDML_Anchor"));
	anchor->set_meta(godot::StringName("__gdml_anchor"), true);
	fragment->anchor_id = (ObjectID)anchor->get_instance_id();
	out_nodes.push_back(anchor);

	std::weak_ptr<FragmentContext> weak_fragment = fragment;

	std::function<void()> render_callback = [weak_fragment, this]() {
		std::shared_ptr<FragmentContext> fragment = weak_fragment.lock();

		if (!fragment) {
			return;
		}

		Object *parent_obj = ObjectDB::get_instance(fragment->parent_id);
		if (!parent_obj) {
			print_line("Trying to render for-statement with non-existent or invalid parent node");
			return;
		}

		Node *parent = Object::cast_to<Node>(parent_obj);
		Node *anchor = Object::cast_to<Node>(ObjectDB::get_instance(fragment->anchor_id));

		if (!parent || !anchor) {
			print_line("Something went wrong when rendering a statement: parent or anchor is null");
			return;
		}

		fragment->dispose_children();

		std::vector<Node *> old_nodes = *fragment->rendered_nodes;
		fragment->rendered_nodes->clear();

		std::shared_ptr<Scope> prev_scope = m_context->m_current_scope;
		m_context->m_current_scope = fragment->scope;
		m_fragments_stack.push_back(fragment);
		if (fragment->statement->type == GDMLNodeType::FOR_STATEMENT) {
			renderForStatement(static_cast<ForStatementNode *>(fragment->statement), fragment->rendered_nodes);
		} else if (fragment->statement->type == GDMLNodeType::IF_STATEMENT) {
			renderIfStatement(static_cast<IfStatementNode *>(fragment->statement), fragment->rendered_nodes);
		}
		m_fragments_stack.pop_back();
		m_context->m_current_scope = prev_scope;

		std::vector<gdml::DiffItem> diff;
		std::map<godot::ObjectID, godot::ObjectID> remaps;
		gdml::Differ differ;
		differ.diff(old_nodes, *fragment->rendered_nodes, parent, diff, remaps);

		std::vector<FragmentContext *> fragment_stack;
		fragment_stack.push_back(fragment.get());
		while (!fragment_stack.empty()) {
			FragmentContext *f = fragment_stack.back();
			fragment_stack.pop_back();

			auto it = remaps.find(f->parent_id);
			if (it != remaps.end()) {
				f->parent_id = it->second;
			}

			it = remaps.find(f->anchor_id);
			if (it != remaps.end()) {
				f->anchor_id = it->second;
			}

			for (Node *&n : *f->rendered_nodes) {
				it = remaps.find((ObjectID)n->get_instance_id());
				if (it != remaps.end()) {
					n = Object::cast_to<Node>(ObjectDB::get_instance(it->second));
				}
			}

			for (std::shared_ptr<traversal::FragmentContext> &c : f->children) {
				fragment_stack.push_back(c.get());
			}
		}

		gdml::TreePatcher patcher;
		patcher.patch(diff);

		for (std::pair<const godot::ObjectID, godot::ObjectID> it : remaps) {
			Object *o = ObjectDB::get_instance(it.first);
			if (o) {
				Node *n = Object::cast_to<Node>(o);
				if (n) {
					n->queue_free();
				}
			}
		}

		int32_t base_index = anchor->get_index() - fragment->rendered_nodes->size();
		int32_t idx = 0;
		for (Node *node : *fragment->rendered_nodes) {
			if (node) {
				parent->move_child(node, base_index + idx);
			}
			idx++;
		}

		old_nodes.clear();
		
		m_context->m_deferred_actions.push_back(std::make_unique<DeferredSignalUpdate>(
			m_context.get(),
			m_result
		));

		for (std::unique_ptr<DeferredAction> &a : m_context->m_deferred_actions) {
			auto dec_action = dynamic_cast<DeferredDecoratorCall *>(a.get());
			if (dec_action) {
				auto it = remaps.find(dec_action->m_node);
				if (it != remaps.end()) {
					dec_action->m_node = it->second;
				}
			}
		}

		m_context->flush_deferred_actions();
	};

	for (String &var_name : m_context->get_scope()->get_active_reactive_variables()) {
		m_context->m_deferred_actions.push_back(std::make_unique<DeferredAtomRenderCallbackAction>(
			m_context.get(),
			var_name,
			fragment,
			render_callback
		));
	}
}

void TreeRendererVisitor::renderForStatement(godot::ForStatementNode *statement, const std::shared_ptr<std::vector<godot::Node *>> &nodes) {
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
					Node *result_node = visitNodeDefinition(*item);
					if (result_node) {
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
					Node *result_node = visitNodeDefinition(*item);
					if (result_node) {
						nodes->push_back(result_node);
					}
				}
			}
		} else {
			for (int32_t i = 0; i < (int32_t)range_variant; i++) {
				m_context->get_scope()->set_variable(iter_name, i);
				for (std::unique_ptr<NodeDefinitionNode> &item : statement->statements) {
					Node *result_node = visitNodeDefinition(*item);
					if (result_node) {
						nodes->push_back(result_node);
					}
				}
			}
		}
	} else {
		m_context->push_error("Cannot resolve range expression of for-statement", *statement->range);
	}
}

void TreeRendererVisitor::renderIfStatement(godot::IfStatementNode *statement, const std::shared_ptr<std::vector<godot::Node *>> &nodes) {
	Variant truthy_variant;
	if (m_prop_helper->resolve(truthy_variant, *statement->expression, truthy_variant)) {
		if (truthy_variant) {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement->if_statements) {
				Node *result_node = visitNodeDefinition(*item);
				if (result_node) {
					nodes->push_back(result_node);
				}
			}
		} else {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement->else_statements) {
				Node *result_node = visitNodeDefinition(*item);
				if (result_node) {
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
		m_context->m_deferred_actions.push_back(std::make_unique<DeferredSetPropertyAction>(
			m_prop_helper.get(),
			root_variant,
			variant,
			prop_key,
			value_node
		));
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

} //namespace traversal

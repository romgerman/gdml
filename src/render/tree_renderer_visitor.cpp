#include "tree_renderer_visitor.h"
#include "gdml_renderer.h"

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

using namespace godot;
using namespace parser;

namespace traversal {

void Scope::set_variable(godot::String p_name, godot::Variant p_value) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			current_scope->m_local_variables[p_name] = p_value;
			return;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	m_local_variables[p_name] = p_value;
}

bool Scope::has_variable(godot::String p_name) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			return true;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return false;
}

godot::Variant Scope::get_variable(godot::String p_name) {
	Scope *current_scope = this;

	do {
		if (current_scope->m_local_variables.find(p_name) != current_scope->m_local_variables.end()) {
			return current_scope->m_local_variables[p_name];
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return godot::Variant();
}

void Scope::set_parent_node(Node *p_node) {
	m_parent_node = p_node;
}

Node *Scope::get_parent_node() {
	Scope *current_scope = this;

	do {
		if (current_scope->m_parent_node) {
			return current_scope->m_parent_node;
		}

		current_scope = current_scope->m_parent_scope.get();
	} while (current_scope != nullptr);

	return nullptr;
}

std::shared_ptr<Scope> Scope::get_parent_scope() {
	return m_parent_scope;
}

std::shared_ptr<Scope> Scope::create_scope() {
	std::shared_ptr<Scope> scope = std::make_shared<Scope>();
	scope->m_parent_scope = shared_from_this();
	return scope;
}

#pragma region Context

void TreeRendererVisitorContext::set_variable(String &p_name, Variant p_value) {
	if (m_variables.find(p_name) != m_variables.end()) {
		Ref<reactive::GDMLAtom> atom = m_variables[p_name];
		atom->set_value(p_value);
	} else {
		Ref<reactive::GDMLAtom> atom = Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
		atom->create(p_name, p_value);
		m_variables[p_name] = atom;
	}
}

Ref<reactive::GDMLAtom> TreeRendererVisitorContext::get_variable(String &p_name) {
	if (m_variables.find(p_name) != m_variables.end()) {
		return m_variables[p_name];
	} else {
		return nullptr;
	}
}

bool TreeRendererVisitorContext::has_variable(String &p_name) {
	return m_variables.find(p_name) != m_variables.end();
}

bool TreeRendererVisitorContext::has_global_signal(godot::String &p_name) {
	return m_signals.find(p_name) != m_signals.end();
}

void TreeRendererVisitorContext::push_active_context() {
	m_active_variables.push_back(std::map<String, bool>());
	m_active_context_index++;
}

void TreeRendererVisitorContext::pop_active_context() {
	m_active_variables.pop_back();
	m_active_context_index--;
}

void TreeRendererVisitorContext::set_active_variable(String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = true;
}

std::vector<Ref<reactive::GDMLAtom>> TreeRendererVisitorContext::get_active_variables() {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return std::vector<Ref<reactive::GDMLAtom>>();
	}
	
	std::vector<Ref<reactive::GDMLAtom>> out;

	for (std::pair<String, bool> pair : m_active_variables[m_active_context_index]) {
		if (pair.second) {
			out.push_back(get_variable(pair.first));
		}
	}

	return out;
}

void TreeRendererVisitorContext::reset_active_variable(String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = false;
}

void TreeRendererVisitorContext::flush_deferred_properties() {
	for (std::function<void()> &fn : m_deferred_properties) {
		fn();
	}

	m_deferred_properties.clear();
}

Scope *TreeRendererVisitorContext::get_scope() {
	return m_current_scope.get();
}

void TreeRendererVisitorContext::push_scope() {
	m_current_scope = m_current_scope->create_scope();
}

void TreeRendererVisitorContext::pop_scope() {
	m_current_scope = m_current_scope->get_parent_scope();
}

TreeRendererVisitorContext::TreeRendererVisitorContext() {
	m_global_scope = std::make_shared<Scope>();
	m_current_scope = m_global_scope;
}

#pragma endregion

#pragma region Renderer

TreeRendererVisitor::TreeRendererVisitor(Ref<GDMLResult> result, godot::Node *p_root) {
	m_result = result;
	m_root_node = p_root;
	m_variant_type_map = {
		{ "Vector2", Variant::Type::VECTOR2 },
		{ "Vector2i", Variant::Type::VECTOR2I },
		{ "Rect2", Variant::Type::RECT2 },
		{ "Rect2i", Variant::Type::RECT2I },
		{ "Vector3", Variant::Type::VECTOR3 },
		{ "Vector3i", Variant::Type::VECTOR3I },
		{ "Transform2D", Variant::Type::TRANSFORM2D },
		{ "Vector4", Variant::Type::VECTOR4 },
		{ "Vector4i", Variant::Type::VECTOR4I },
		{ "Plane", Variant::Type::PLANE },
		{ "Quaternion", Variant::Type::QUATERNION },
		{ "AABB", Variant::Type::AABB },
		{ "Basis", Variant::Type::BASIS },
		{ "Transform3D", Variant::Type::TRANSFORM3D },
		{ "Projection", Variant::Type::PROJECTION },

		{ "Color", Variant::Type::COLOR },
		{ "StringName", Variant::Type::STRING_NAME },
		{ "NodePath", Variant::Type::NODE_PATH },
		{ "RID", Variant::Type::RID },
		{ "Callable", Variant::Type::CALLABLE },
		{ "Signal", Variant::Type::SIGNAL },
		{ "Dictionary", Variant::Type::DICTIONARY },
		{ "Array", Variant::Type::ARRAY },

		{ "PackedByteArray", Variant::Type::PACKED_BYTE_ARRAY },
		{ "PackedInt32Array", Variant::Type::PACKED_INT32_ARRAY },
		{ "PackedInt64Array", Variant::Type::PACKED_INT64_ARRAY },
		{ "PackedFloat32Array", Variant::Type::PACKED_FLOAT32_ARRAY },
		{ "PackedFloat64Array", Variant::Type::PACKED_FLOAT64_ARRAY },
		{ "PackedStringArray", Variant::Type::PACKED_STRING_ARRAY },
		{ "PackedVector2Array", Variant::Type::PACKED_VECTOR2_ARRAY },
		{ "PackedVector3Array", Variant::Type::PACKED_VECTOR3_ARRAY },
		{ "PackedColorArray", Variant::Type::PACKED_COLOR_ARRAY },
		{ "PackedVector4Array", Variant::Type::PACKED_VECTOR4_ARRAY },
	};
}

TreeRendererVisitorResult TreeRendererVisitor::visit(const GdmlNode &root_node) {
	m_context = std::make_unique<TreeRendererVisitorContext>();
	m_context->get_scope()->set_parent_node(m_root_node);

	if (root_node.type == GDMLNodeType::SOURCE_FILE) {
		TreeRendererVisitorResult result;
		m_context->push_active_context();
		{
			visitSourceFile((SourceFileNode &)root_node, result);
			m_context->flush_deferred_properties();
			result.variables = m_context->m_variables;
			result.signals = m_context->m_signals;
		}
		m_context->pop_active_context();
		return result;
	}

	printError("Cannot render tree. The tree is malformed", root_node);
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

			m_context->push_active_context();
			m_context->push_scope();
			{
				Variant result_node = visitNodeDefinition(*node_def);
				if (result_node) {
					result.nodes.push_back(result_node);
				}
			}
			m_context->pop_scope();
			m_context->pop_active_context();
		} break;
		case GDMLNodeType::VARIABLE_DEFINITION: {
			Variant value;
			VariableDefinitionNode *var = static_cast<VariableDefinitionNode *>(definition->statement.get());

			if (var->value) {
				if (resolveExpression(value, *var->value, value)) {
					m_context->set_variable(var->name->name, value);
				} else {
					printError("Couldn't resolve variable expression", *var);
				}
			} else {
				m_context->set_variable(var->name->name, Variant());
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
			visitForStatement(*statement, result.nodes);
			m_context->pop_scope();
		} break;
		case GDMLNodeType::IF_STATEMENT: {
			IfStatementNode *statement = static_cast<IfStatementNode *>(definition->statement.get());
			m_context->push_scope();
			visitIfStatement(*statement, result.nodes);
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
				printError("Cannot load import path " + import_path, node);
				return nullptr;
			}
	
			Node *base_object = scene->instantiate();
			assert(base_object);
			root_variant = base_object;
		} else if (import_path.ends_with(".gd")) {
			Ref<Script> script = ResourceLoader::get_singleton()->load(import_path, "Script");
			if (script.is_null()) {
				printError("Cannot load script associated with type '" + node_name + "' by path " + import_path, node);
				return nullptr;
			}
			root_variant = ClassDB::instantiate(script->get_instance_base_type());
			Object *base_object = root_variant.get_validated_object();
			assert(base_object);
			base_object->set_script(script);
		} else if (import_path.ends_with(".gdml")) {
			Ref<Script> script = ResourceLoader::get_singleton()->load(import_path, "Script");
			if (script.is_null()) {
				printError("Cannot load markup associated with type '" + node_name + "' by path " + import_path, node);
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
								if (resolveExpression(root_variant, *prop->value, out)) {
									component_scope->set_variable(param_name, out);
								} else {
									printError("Cannot resolve expression while rendering a component", *prop);
								}
							}
						}
					}
				}

				m_context->m_current_scope = component_scope;
				root_variant = visitNodeDefinition(*component->node);
				m_context->m_current_scope = current_scope;
			} else {
				printError("Cannot find an imported component with name '" + node_name + "' [" + import_path + "]", node);
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
						if (resolveExpression(root_variant, *prop->value, out)) {
							component_scope->set_variable(param_name, out);
						} else {
							printError("Cannot resolve expression while rendering a component", *prop);
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
				printError("Class with name '" + node_name + "' is not found", node);
				return nullptr;
			}
	
			String path = class_info.get("path", "");
			Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
			if (script.is_null()) {
				printError("Cannot load script associated with type '" + node_name + "' by path " + path, node);
				return nullptr;
			}
			root_variant = ClassDB::instantiate(script->get_instance_base_type());
			Object *base_object = root_variant.get_validated_object();
			assert(base_object);
			base_object->set_script(script);
		} else {
			if (!ClassDB::can_instantiate(node_name)) {
				printError("Cannot construct node of type '" + node_name + "'", node);
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
			printError("Object is not a node", node);
			Object *obj = Object::cast_to<Object>(root_variant);
			if (obj && !Object::cast_to<RefCounted>(obj)) {
				memdelete(obj);
			}
			return nullptr;
		}
	} else {
		printError("Variant type is not an object", node);
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
				printError("Trying to set theme override on a non-Control node", *property);
			}
		} else {
			if (property->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				trySetVariantProperty(root_variant, root_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
				MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, root_variant, *member_access, *property->value.get());
			} else if (property->name->type == GDMLNodeType::SIGNAL_EXPRESSION) {
				const SignalExpressionNode *signal_expression = static_cast<SignalExpressionNode *>(property->name.get());
				if (signal_expression->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					const String signal_name = static_cast<IdentifierNode *>(signal_expression->name.get())->name;
					if (property->value->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<IdentifierNode *>(property->value.get())->name);
					} else if (property->value->type == GDMLNodeType::STRING_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<StringNode *>(property->value.get())->value);
					} else {
						printError("Cannot connect signal. Callable name should be an identifier or a string", *property);
					}
				} else {
					printError("Cannot connect signal. Wrong name expression type", *property);
				}
			}
		}
	}

	for (const std::unique_ptr<VariableDefinitionNode> &var : node.variables) {
		Variant value;
		if (resolveExpression(value, *var, value)) {
			m_context->get_scope()->set_variable(var->name->name, value);
		}
	}

	for (const std::unique_ptr<FunctionCallExpressionNode> &dec_fn : node.decorators) {
		if (dec_fn->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
			IdentifierNode *dec_fn_name = static_cast<IdentifierNode *>(dec_fn->name.get());
			String decorator_classname = "GDMLDecorator";

			Dictionary class_info;
			Object *decorator_object = nullptr;

			for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
				if (String(ci.get("base", "")) == decorator_classname) {
					String path = ci.get("path", "");

					if (path.is_empty()) {
						continue;
					}

					Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
					if (script.is_null()) {
						printError("Cannot load decorator script " + path, node);
						continue;
					}

					Variant decorator_variant = ClassDB::instantiate(script->get_instance_base_type());
					decorator_object = decorator_variant.get_validated_object();
					if (!decorator_object) {
						printError("Cannot construct decorator base type", *dec_fn);
						continue;
					}
					decorator_object->set_script(script);

					if (!decorator_object->has_method(StringName("get_decorator_name"))) {
						printError("Decorator script doesn't have required 'get_decorator_name' method " + path, node);
						continue;
					}

					if (String(decorator_object->call(StringName("get_decorator_name"))) == dec_fn_name->name) {
						class_info = ci;
						break;
					} else {
						memdelete(decorator_object);
					}
				}
			}

			if (class_info.is_empty()) {
				printError("Decorator script for '" + dec_fn_name->name + "' not found", node);
				continue;
			}

			Array call_args;
			call_args.push_back(obj_as_node);
			
			Array args;
			for (std::unique_ptr<ExpressionNode> &arg_node : dec_fn->arguments) {
				Variant arg_value;
				if (resolveExpression(root_variant, *arg_node, arg_value)) {
					args.push_back(arg_value);
				} else {
					printError("Cannot resolve decorator argument", *arg_node);
				}
			}

			call_args.push_back(args);

			if (!decorator_object->has_method(StringName("modify"))) {
				memdelete(decorator_object);
				continue;
			} else {
				decorator_object->callv(StringName("modify"), call_args);
			}

			memdelete(decorator_object);
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
			visitForStatement(*for_node, nodes);
			m_context->pop_scope();

			for (Variant &item : nodes) {
				obj_as_node->add_child(Object::cast_to<Node>(item));
			}
		} else if (statement->type == GDMLNodeType::IF_STATEMENT) {
			IfStatementNode *if_node = static_cast<IfStatementNode *>(statement.get());
			std::vector<Variant> nodes;
			m_context->push_scope();
			visitIfStatement(*if_node, nodes);
			m_context->pop_scope();

			for (Variant &item : nodes) {
				obj_as_node->add_child(Object::cast_to<Node>(item));
			}
		}
	}

	return root_variant;
}

void TreeRendererVisitor::visitForStatement(ForStatementNode &statement, std::vector<Variant> &out_nodes) {
	std::shared_ptr<std::vector<Variant>> nodes = std::make_shared<std::vector<Variant>>();
	std::shared_ptr<Scope> scope = m_context->m_current_scope;
	Node* parent = m_context->get_scope()->get_parent_node();
	
	renderForStatement(statement, nodes, out_nodes);

	std::function<void()> render_callback = [parent, nodes, scope, &statement, this]() {
		if (!parent) {
			return;
		}

		for (Variant item : *nodes) {
			Node *node = Object::cast_to<Node>(item);
			if (node) {
				node->queue_free();
			}
		}

		nodes->clear();

		std::vector<Variant> cb_nodes;
		std::shared_ptr<Scope> prev_scope = m_context->m_current_scope;
		m_context->m_current_scope = scope;
		renderForStatement(statement, nodes, cb_nodes);
		m_context->m_current_scope = prev_scope;

		for (auto result_node : cb_nodes) {
			Node *node = Object::cast_to<Node>(result_node);
			if (node) {
				parent->add_child(node);
				node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
				nodes->push_back(result_node);
			}
		}
		
		m_context->flush_deferred_properties();
		updateSignals();
	};

	for (Ref<reactive::GDMLAtom> atom : m_context->get_active_variables()) {
		atom->add_callback(render_callback);
	}
}

void TreeRendererVisitor::renderForStatement(ForStatementNode &statement, const std::shared_ptr<std::vector<Variant>> &nodes, std::vector<Variant> &out_nodes) {
	String iter_name;

	if (statement.iter_name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		iter_name = static_cast<IdentifierNode &>(*statement.iter_name).name;
	} else {
		printError("Cannot use non-identifier expressions in for-statement iterator", statement);
		return;
	}

	String iter_name2;
	bool has_iter2 = false;

	if (statement.iter_name2) {
		if (statement.iter_name2->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
			iter_name2 = static_cast<IdentifierNode &>(*statement.iter_name2).name;
			has_iter2 = true;
		} else {
			printError("Cannot use non-identifier expressions in for-statement iterator", statement);
			return;
		}
	}

	Variant range_variant;
	if (resolveExpression(range_variant, *statement.range, range_variant)) {
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
				for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
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
				for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
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
		printError("Cannot resolve range expression of for-statement", *statement.range);
	}
}

void TreeRendererVisitor::visitIfStatement(IfStatementNode &statement, std::vector<Variant> &out_nodes) {
	std::shared_ptr<std::vector<Variant>> nodes = std::make_shared<std::vector<Variant>>();
	std::shared_ptr<Scope> scope = m_context->m_current_scope;
	Node* parent = m_context->get_scope()->get_parent_node();
	
	renderIfStatement(statement, nodes, out_nodes);

	std::function<void()> render_callback = [parent, nodes, scope, &statement, this]() {
		if (!parent) {
			return;
		}

		for (Variant item : *nodes) {
			Node *node = Object::cast_to<Node>(item);
			if (node) {
				node->queue_free();
			}
		}

		nodes->clear();

		std::vector<Variant> cb_nodes;
		std::shared_ptr<Scope> prev_scope = m_context->m_current_scope;
		m_context->m_current_scope = scope;
		renderIfStatement(statement, nodes, cb_nodes);
		m_context->m_current_scope = prev_scope;

		for (auto result_node : cb_nodes) {
			Node *node = Object::cast_to<Node>(result_node);
			if (node) {
				parent->add_child(node);
				node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
				nodes->push_back(result_node);
			}
		}
		
		m_context->flush_deferred_properties();
		updateSignals();
	};

	for (Ref<reactive::GDMLAtom> atom : m_context->get_active_variables()) {
		atom->add_callback(render_callback);
	}
}

void TreeRendererVisitor::renderIfStatement(IfStatementNode &statement, const std::shared_ptr<std::vector<Variant>> &nodes, std::vector<Variant> &out_nodes) {
	Variant truthy_variant;
	if (resolveExpression(truthy_variant, *statement.expression, truthy_variant)) {
		if (truthy_variant) {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement.if_statements) {
				m_context->push_active_context();
				Variant result_node = visitNodeDefinition(*item);
				m_context->pop_active_context();
				if (result_node) {
					out_nodes.push_back(result_node);
					nodes->push_back(result_node);
				}
			}
		} else {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement.else_statements) {
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
		printError("Cannot resolve if-statement expression", *statement.expression);
	}
}

void TreeRendererVisitor::visitSignalDefinition(SignalDefinitionNode &definition) {
	m_context->m_signals[definition.name->name] = TreeRendererVisitorContext::SignalTarget();
}

void TreeRendererVisitor::visitImportStatement(godot::ImportStatementNode &statement) {
	m_context->m_imports[statement.name->name] = statement.from->value;
}

bool TreeRendererVisitor::trySetVariantProperty(Variant &root_variant, Variant &variant, String &prop_key, ExpressionNode &value_node) {
	if (doesExpressionHaveSelf(value_node)) {
		std::function<void()> deferred_set = [this, root_variant, variant, prop_key, &value_node]() {
			setVariantProperty(const_cast<Variant &>(root_variant), const_cast<Variant &>(variant), const_cast<String &>(prop_key), value_node);
		};

		m_context->m_deferred_properties.push_back(deferred_set);
		return true;
	} else{
		return setVariantProperty(root_variant, variant, prop_key, value_node);
	}
}

bool TreeRendererVisitor::setVariantProperty(Variant &root_variant, Variant &variant, String &prop_key, ExpressionNode &value_node) {
	Variant out_value;
	if (resolveExpression(root_variant, value_node, out_value)) {
		variant.set(prop_key, out_value);
		return true;
	} else {
		return false;
	}
}

bool TreeRendererVisitor::setVariantPropertyDeep(Variant &root_variant, Variant &variant, MemberExpressionNode &member_expression, ExpressionNode &value_node) {
	Variant previous_variant;
	Variant current_variant = variant;

	bool is_valid = false;
	// for (auto it = path.begin(); it != path.end() - 1; ++it) {
	// 	previous_variant = current_variant;
	// 	current_variant = current_variant.get(it->get()->name, &is_valid);

	// 	if (!is_valid) {
	// 		print_error("Cannot get property " + it->get()->name + " in path");
	// 		return false;
	// 	}
	// }

	// String &variant_property = path.back()->name;
	// if (setVariantProperty(root_variant, current_variant, variant_property, value_node)) {
	// 	const String &previous_property = (path.end() - 2)->get()->name;
	// 	previous_variant.set(previous_property, current_variant, &is_valid);
	// }

	return is_valid;
}

bool TreeRendererVisitor::getPropertyValue(Variant &variant, String &prop_key, Variant &value) {
	if (prop_key == "self") {
		value = variant;
		return true;
	}

	bool is_valid = false;

	if (!is_valid) {
		if (m_context->get_scope()->has_variable(prop_key)) {
			value = m_context->get_scope()->get_variable(prop_key);
			is_valid = true;
		}
	}

	if (!is_valid) {
		if (m_context->has_variable(prop_key)) {
			Ref<reactive::GDMLAtom> atom = m_context->get_variable(prop_key);
			value = atom->get_value();
			is_valid = true;
			m_context->set_active_variable(prop_key);
		}
	}

	if (!is_valid) {
		const Variant prop_value = variant.get(prop_key, &is_valid);
		if (is_valid) {
			value = prop_value;
		}
	}

	if (!is_valid) {
		Dictionary class_info;
	
		for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
			if (String(ci.get("class", "")) == prop_key) {
				class_info = ci;
				break;
			}
		}

		if (!class_info.is_empty()) {
			String path = class_info.get("path", "");
			Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
			if (script.is_valid()) {
				value = script;
				is_valid = true;
			}
		}
	}

	if (!is_valid) {
		if (Engine::get_singleton()->has_singleton(prop_key)) {
			value = Engine::get_singleton()->get_singleton(prop_key);
			is_valid = true;
		}
	}

	return is_valid;
}

bool TreeRendererVisitor::getDeepPropertyValue(Variant &root_variant, MemberExpressionNode &member_expression, Variant &value) {
	std::unique_ptr<ExpressionNode> &expression = member_expression.expression;
	std::unique_ptr<IdentifierNode> &prop_name = member_expression.identifier;
	
	if (expression->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode *id_expr = static_cast<IdentifierNode *>(expression.get());
		const StringName &value_name = prop_name->name;

		// Check if it's a constant of a basic Variant type
		if (m_variant_type_map.count(id_expr->name)) {
			Variant ret;
			internal::gdextension_interface_variant_get_constant_value(
					(GDExtensionVariantType)m_variant_type_map[id_expr->name],
					value_name._native_ptr(),
					&ret);
			if (ret.get_type() != Variant::NIL) {
				value = ret;
				return true;
			}
		}

		// Check if it's a class and has the next path segment as a constant
		if (ClassDB::class_exists(id_expr->name)) {
			if (ClassDB::class_has_integer_constant(id_expr->name, value_name)) {
				value = ClassDB::class_get_integer_constant(id_expr->name, value_name);
				return true;
			}
		}
	}

	Variant base_variant;

	if (expression) {
		if (!resolveExpression(root_variant, *expression, base_variant)) {
			return false;
		}
	} else {
		return false;
	}

	bool is_valid = false;
	
	// We have a value go check next
	Variant next_value = base_variant.get(prop_name->name, &is_valid);

	if (is_valid) {
		value = next_value;
		return true;
	}

	Object *obj = base_variant.get_validated_object();
	if (obj) {
		const String class_name = obj->get_class();

		if (ClassDB::class_has_integer_constant(class_name, prop_name->name)) {
			value = ClassDB::class_get_integer_constant(class_name, prop_name->name);
			return true;
		}
	}

	return false;
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

		if (m_context->has_variable(node.name)) {
			subscribeToVariable(node.name, prop_key, variant);
		}

		Variant out_value;
		if (getPropertyValue(root_variant, node.name, out_value)) {
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
		if (getDeepPropertyValue(root_variant, node, out_value)) {
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
			printError("Cannot resolve expression", node);
		}
	} else if (value_node.type == GDMLNodeType::RESOURCE_DEFINITION) {
		const ResourceDefinitionNode &node = (ResourceDefinitionNode &)value_node;
		Variant resource_variant;

		if (m_variant_type_map.count(node.name->name)) {
			GDExtensionCallError error;
			internal::gdextension_interface_variant_construct(
					(GDExtensionVariantType)m_variant_type_map[node.name->name],
					resource_variant._native_ptr(),
					nullptr,
					0,
					&error);

			if (error.error != GDEXTENSION_CALL_OK) {
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
				setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
				MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, resource_variant, *member_access, *property->value.get());
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

			if (m_context->has_variable(name_node->name)) {
				Ref<reactive::GDMLAtom> atom = m_context->get_variable(name_node->name);
				Variant value = atom->get_value();

				if (value.get_type() == Variant::Type::CALLABLE) {
					Callable callable = value;
					Array args;

					for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
						Variant arg_value;
						if (resolveExpression(root_variant, *expr.get(), arg_value)) {
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

bool TreeRendererVisitor::resolveExpression(Variant &root_variant, ExpressionNode &expr_node, Variant &value) {
	switch (expr_node.type) {
		case GDMLNodeType::STRING_EXPRESSION: {
			StringNode &node = static_cast<StringNode &>(expr_node);
			value = node.value;
		} break;
		case GDMLNodeType::NUMBER_EXPRESSION: {
			NumberNode node = static_cast<NumberNode &>(expr_node);
			value = Variant(node.isFloat ? node.getFloat() : node.getInteger());
		} break;
		case GDMLNodeType::BOOLEAN_EXPRESSION: {
			BooleanNode &node = static_cast<BooleanNode &>(expr_node);
			value = node.value;
		} break;
		case GDMLNodeType::IDENTIFIER_EXPRESSION: {
			IdentifierNode &node = static_cast<IdentifierNode &>(expr_node);
			if (node.name == "self") {
				value = root_variant;
				return true;
			}

			Variant out_value;
			if (getPropertyValue(root_variant, node.name, out_value)) {
				value = out_value;
			} else {
				print_error("Couldn't access property \"" + node.name + "\" to get value");
				return false;
			}
		} break;
		case GDMLNodeType::ARRAY_EXPRESSION: {
			ArrayExpressionNode &node = static_cast<ArrayExpressionNode &>(expr_node);
			Array out_arr;

			for (const std::unique_ptr<ExpressionNode> &item_node : node.values) {
				Variant out_value;
				if (resolveExpression(root_variant, *item_node, out_value)) {
					out_arr.push_back(out_value);
				}
			}

			value = out_arr;
		} break;
		case GDMLNodeType::DICTIONARY_EXPRESSION: {
			DictionaryExpressionNode &node = static_cast<DictionaryExpressionNode &>(expr_node);
			Dictionary out_dict;

			for (const std::unique_ptr<PropertyDefinitionNode> &item_node : node.values) {
				if (item_node->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					String &name = static_cast<IdentifierNode *>(item_node->name.get())->name;
					Variant out_value;
					if (resolveExpression(root_variant, *item_node->value.get(), out_value)) {
						out_dict[name] = out_value;
					}
				}
			}

			value = out_dict;
		} break;
		case GDMLNodeType::MEMBER_ACCESS_EXPRESSION: {
			MemberExpressionNode &node = static_cast<MemberExpressionNode &>(expr_node);
			Variant out_value;
			if (getDeepPropertyValue(root_variant, node, out_value)) {
				value = out_value;
			} else {
				printError("Cannot resolve member expression", node);
				return false;
			}
		} break;
		case GDMLNodeType::RESOURCE_DEFINITION: {
			ResourceDefinitionNode &node = static_cast<ResourceDefinitionNode &>(expr_node);
			Variant resource_variant;

			if (m_variant_type_map.count(node.name->name)) {
				GDExtensionCallError error;
				internal::gdextension_interface_variant_construct(
						(GDExtensionVariantType)m_variant_type_map[node.name->name],
						resource_variant._native_ptr(),
						nullptr,
						0,
						&error);

				if (error.error != GDEXTENSION_CALL_OK) {
					print_error("Error constructing variant of type " + node.name->name);
					return false;
				}
			} else {
				if (ClassDB::class_exists(node.name->name)) {
					resource_variant = ClassDB::instantiate(node.name->name);
				} else {
					print_error("Error instantiating class of type " + node.name->name);
					return false;
				}
			}

			for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
				if (property->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
					setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
				} else if (property->name->type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
					MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
					setVariantPropertyDeep(root_variant, resource_variant, *member_access, *property->value.get());
				}
			}

			for (const std::unique_ptr<FunctionCallExpressionNode> &dec_fn : node.decorators) {
				if (dec_fn->name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode *dec_fn_name = static_cast<IdentifierNode *>(dec_fn->name.get());
					String decorator_classname = "GDMLDecorator_" + dec_fn_name->name;

					Dictionary class_info;

					for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
						if (String(ci.get("class", "")) == decorator_classname) {
							class_info = ci;
							break;
						}
					}

					if (class_info.is_empty()) {
						printError("Class with name '" + decorator_classname + "' is not found", node);
						continue;
					}

					String path = class_info.get("path", "");
					Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
					if (script.is_null()) {
						printError("Cannot load script associated with type '" + decorator_classname + "' by path " + path, node);
						continue;
					}
					Variant decorator_variant = ClassDB::instantiate(script->get_instance_base_type());
					Object *decorator_object = decorator_variant.get_validated_object();
					if (!decorator_object) {
						printError("Cannot construct decorator base class", *dec_fn);
						continue;
					}
					decorator_object->set_script(script);

					Array call_args;
					call_args.push_back(resource_variant);
					
					Array args;
					for (std::unique_ptr<ExpressionNode> &arg_node : dec_fn->arguments) {
						Variant arg_value;
						if (resolveExpression(root_variant, *arg_node, arg_value)) {
							args.push_back(arg_value);
						} else {
							printError("Cannot resolve decorator argument", *arg_node);
						}
					}

					call_args.push_back(args);

					if (!decorator_object->has_method(StringName("modify_resource"))) {
						printError("Decorator doesn't have a 'modify_resource' method", *dec_fn);
						memdelete(decorator_object);
						continue;
					} else {
						decorator_object->callv(StringName("modify_resource"), call_args);
					}

					memdelete(decorator_object);
				}
			}

			value = resource_variant;
		} break;
		case GDMLNodeType::FUNCTION_CALL: {
			const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)expr_node;
			
			Array args;
	
			for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
				Variant arg_value;
				if (resolveExpression(root_variant, *expr.get(), arg_value)) {
					args.push_back(arg_value);
				} else {
					print_error("Unsupported argument type '" + String::num_int64((int)expr->type) + "' in function call");
				}
			}

			if (node.name->type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *name_node = static_cast<IdentifierNode *>(node.name.get());

				Variant target;
				if (!resolveExpression(root_variant, *node.name, target)) {
					printError("Cannot resolve function call", node);
					return false;
				}

				if (target.get_type() != Variant::CALLABLE) {
					printError("Cannot call non-Callable",node);
					return false;
				}

				Callable callable = target;
				value = callable.callv(args);
			} else if (node.name->type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
				MemberExpressionNode *member_node = static_cast<MemberExpressionNode *>(node.name.get());

				Variant target;
				if (!resolveExpression(root_variant, *member_node->expression, target)) {
					printError("Cannot resolve function call", node);
					return false;
				}

				Object *obj = target.get_validated_object();
				if (obj) {
					value = obj->callv(member_node->identifier->name, args);
				}
			} else {
				printError("Unsupported call target", node);
				return false;
			}
		} break;
		case GDMLNodeType::BINARY_EXPRESSION: {
			BinaryExpressionNode &node = static_cast<BinaryExpressionNode &>(expr_node);
			
			Variant left;
			if (resolveExpression(root_variant, *node.left, left)) {
				Variant right;
				if (resolveExpression(root_variant, *node.right, right)) {
					switch (node.op)
					{
					case GDMLOperator::AND:
						value = left && right;
						break;
					case GDMLOperator::OR:
						value = left || right;
						break;
					case GDMLOperator::PLUS:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left + (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left + (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left + (float_t)right;
						} else if (left.get_type() == Variant::STRING || right.get_type() == Variant::STRING) {
							value = String(left) + String(right);
						}
						break;
					case GDMLOperator::MINUS:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left - (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left - (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left - (float_t)right;
						}
						break;
					case GDMLOperator::MUL:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left * (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left * (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left * (float_t)right;
						}
						break;
					case GDMLOperator::DIV:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left / (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left / (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left / (float_t)right;
						}
						break;
					case GDMLOperator::LESS_THAN:
						value = left < right;
						break;
					case GDMLOperator::LESS_OR_EQUAL_THAN:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left <= (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left <= (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left <= (float_t)right;
						}
						break;
					case GDMLOperator::GREATER_THAN:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left > (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left > (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left > (float_t)right;
						}
						break;
					case GDMLOperator::GREATER_OR_EQUAL_THAN:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left >= (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left >= (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left >= (float_t)right;
						}
						break;
					case GDMLOperator::EQUAL:
						if (left.get_type() == Variant::INT && right.get_type() == Variant::INT)  {
							value = (int64_t)left == (int64_t)right;
						} else if (left.get_type() == Variant::FLOAT && right.get_type() == Variant::FLOAT) {
							value = (float_t)left == (float_t)right;
						} else if ((left.get_type() == Variant::INT && right.get_type() == Variant::FLOAT) ||
									(left.get_type() == Variant::FLOAT && right.get_type() == Variant::INT)) {
							value = (float_t)left == (float_t)right;
						} else {
							value = left == right;
						}
						break;
					case GDMLOperator::NOT_EQUAL:
						value = left != right;
						break;
					}
				} else {
					printError("Cannot resolve right part of binary expression", node);
				}
			} else {
				printError("Cannot resolve left part of binary expression", node);
			}

		} break;
		case GDMLNodeType::SUBSCRIPT_EXPRESSION: {
			const SubscriptExpressionNode &node = (SubscriptExpressionNode &)expr_node;

			Variant v_object;
			if (!resolveExpression(root_variant, *node.object, v_object)) {
				printError("Cannot resolve object", node);
				return false;
			}

			Variant v_index;
			if (!resolveExpression(root_variant, *node.index, v_index)) {
				printError("Cannot resolve index", node);
				return false;
			}
			
			if (v_object.get_type() == Variant::ARRAY) {
				Array obj = v_object;
				int64_t index = v_index;
				value = obj[index];
			} else if (v_object.get_type() == Variant::DICTIONARY) {
				Dictionary obj = v_object;
				value = obj[v_index];
			}
		} break;
		default: {
			printError("Cannot resolve unknown expression type", expr_node);
			return false;
		}
	}

	return true;
}

bool TreeRendererVisitor::doesExpressionHaveSelf(ExpressionNode &expr_node) {
	if (expr_node.type == GDMLNodeType::IDENTIFIER_EXPRESSION) {
		return static_cast<IdentifierNode &>(expr_node).name == "self";
	} else if (expr_node.type == GDMLNodeType::MEMBER_ACCESS_EXPRESSION) {
		return doesExpressionHaveSelf(*static_cast<MemberExpressionNode &>(expr_node).expression);
	} else if (expr_node.type == GDMLNodeType::ARRAY_EXPRESSION) {
		for (std::unique_ptr<ExpressionNode> &value : static_cast<ArrayExpressionNode &>(expr_node).values) {
			return doesExpressionHaveSelf(*value);
		}
	} else if (expr_node.type == GDMLNodeType::DICTIONARY_EXPRESSION) {
		for (std::unique_ptr<PropertyDefinitionNode> &value : static_cast<DictionaryExpressionNode &>(expr_node).values) {
			return doesExpressionHaveSelf(*value->value);
		}
	} else if (expr_node.type == GDMLNodeType::RESOURCE_DEFINITION) {
		for (std::unique_ptr<PropertyDefinitionNode> &value : static_cast<ResourceDefinitionNode &>(expr_node).properties) {
			return doesExpressionHaveSelf(*value->value);
		}
	} else if (expr_node.type == GDMLNodeType::FUNCTION_CALL) {
		for (std::unique_ptr<ExpressionNode> &value : static_cast<FunctionCallExpressionNode &>(expr_node).arguments) {
			return doesExpressionHaveSelf(*value);
		}
	}

	return false;
}

void TreeRendererVisitor::subscribeToVariable(String &p_variable, String &p_prop, Variant &variant) {
	Ref<reactive::GDMLAtom> atom = m_context->get_variable(p_variable);
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

void TreeRendererVisitor::printError(const String p_error, const GdmlNode &node) {
	print_error(p_error, " at ", node.location.start.line, ":", node.location.start.column);
}

#pragma endregion

} //namespace traversal

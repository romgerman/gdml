#include "tree_renderer_visitor.h"
#include "gdml_renderer.h"

#include <assert.h>
#include <functional>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

namespace traversal {

#pragma region Context

void TreeRendererVisitorContext::set_data(const godot::String &p_name, godot::Variant p_value) {
	m_data[p_name] = p_value;
}

bool TreeRendererVisitorContext::has_data(godot::String &p_name) {
	return m_data.find(p_name) != m_data.end();
}

godot::Variant TreeRendererVisitorContext::get_data(godot::String &p_name) {
	if (m_data.find(p_name) != m_data.end()) {
		return m_data[p_name];
	} else {
		return nullptr;
	}
}

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

void TreeRendererVisitorContext::push_active_context() {
	m_active_variables.push_back(std::map<godot::String, bool>());
	m_active_context_index++;
}

void TreeRendererVisitorContext::pop_active_context() {
	m_active_variables.pop_back();
	m_active_context_index--;
}

void TreeRendererVisitorContext::set_active_variable(godot::String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = true;
}

std::vector<godot::Ref<reactive::GDMLAtom>> TreeRendererVisitorContext::get_active_variables() {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return std::vector<godot::Ref<reactive::GDMLAtom>>();
	}
	
	std::vector<godot::Ref<reactive::GDMLAtom>> out;

	for (std::pair<String, bool> pair : m_active_variables[m_active_context_index]) {
		if (pair.second) {
			out.push_back(get_variable(pair.first));
		}
	}

	return out;
}

void TreeRendererVisitorContext::reset_active_variable(godot::String &p_name) {
	if (m_active_context_index < 0 || m_active_context_index >= m_active_variables.size()) {
		return;
	}

	m_active_variables[m_active_context_index][p_name] = false;
}

#pragma endregion

#pragma region Renderer

TreeRendererVisitor::TreeRendererVisitor(godot::Ref<godot::GDMLResult> result) {
	m_result = result;
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

	if (root_node.type == GdmlNodeType::SOURCE_FILE) {
		TreeRendererVisitorResult result;
		m_context->push_active_context();
		{
			visitSourceFile((SourceFileNode &)root_node, result);
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
	for (const std::unique_ptr<DefinitionNode> &definition : source_node.statements) {
		switch (definition->statement->type)
		{
		case GdmlNodeType::NODE_DEFINITION: {
			const NodeDefinitionNode *node_def = static_cast<const NodeDefinitionNode *>(definition->statement.get());

			m_context->push_active_context();
			{
				Variant result_node = visitNodeDefinition(*node_def);
				if (result_node) {
					result.nodes.push_back(result_node);
				}
			}
			m_context->pop_active_context();
		} break;
		case GdmlNodeType::VARIABLE_DEFINITION: {
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
		case GdmlNodeType::FOR_STATEMENT: {
			ForStatementNode *statement = static_cast<ForStatementNode *>(definition->statement.get());
			visitForStatement(*statement, result.nodes);
		} break;
		case GdmlNodeType::SIGNAL_DEFINITION: {
			SignalDefinitionNode *def = static_cast<SignalDefinitionNode *>(definition->statement.get());
			visitSignalDefinition(*def);
		} break;
		}
	}
}

Variant TreeRendererVisitor::visitNodeDefinition(const NodeDefinitionNode &node) {
	Variant root_variant;

	if (!ClassDB::class_exists(node.name->name)) {
		Dictionary class_info;

		for (const Dictionary &ci : ProjectSettings::get_singleton()->get_global_class_list()) {
			if (String(ci.get("class", "")) == node.name->name) {
				class_info = ci;
				break;
			}
		}

		if (class_info.is_empty()) {
			printError("Class with name '" + node.name->name + "' is not found", node);
			return nullptr;
		}

		String path = class_info.get("path", "");
		Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
		if (script.is_null()) {
			printError("Cannot load script associated with type '" + node.name->name + "' by path " + path, node);
			return nullptr;
		}
		root_variant = ClassDB::instantiate(script->get_instance_base_type());
		Object *base_object = root_variant.get_validated_object();
		assert(base_object);
		base_object->set_script(script);
	} else {
		if (!ClassDB::can_instantiate(node.name->name)) {
			printError("Cannot construct node of type '" + node.name->name + "'", node);
			return nullptr;
		}

		root_variant = ClassDB::instantiate(node.name->name);
	}

	bool is_node = false;
	Node *obj_as_node;
	if (root_variant.get_type() == Variant::Type::OBJECT) {
		obj_as_node = Object::cast_to<Node>(root_variant);
		if (obj_as_node) {
			is_node = true;
		} else {
			printError("Object is not a node", node);
			return nullptr;
		}
	} else {
		printError("Variant type is not an object", node);
		return nullptr;
	}

	for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
		if (property->is_theme_override) {
			Control *control = Object::cast_to<Control>(root_variant);
			if (control) {
				if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
					setControlThemeOverride(root_variant, root_variant, identifier->name, *property->value.get());
				}
			} else {
				printError("Trying to set theme override on a non-Control node", *property);
			}
		} else {
			if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				setVariantProperty(root_variant, root_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				const MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, root_variant, member_access->path, *property->value.get());
			} else if (property->name->type == GdmlNodeType::SIGNAL_EXPRESSION) {
				const SignalExpressionNode *signal_expression = static_cast<SignalExpressionNode *>(property->name.get());
				if (signal_expression->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
					const String signal_name = static_cast<IdentifierNode *>(signal_expression->name.get())->name;
					if (property->value->type == GdmlNodeType::IDENTIFIER_EXPRESSION || property->value->type == GdmlNodeType::STRING_EXPRESSION) {
						if (property->value->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
							connectSignal(root_variant, root_variant, signal_name, static_cast<IdentifierNode *>(property->value.get())->name);
						} else if (property->value->type == GdmlNodeType::STRING_EXPRESSION) {
							connectSignal(root_variant, root_variant, signal_name, static_cast<StringNode *>(property->value.get())->value);
						}
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
			m_context->set_variable(var->name->name, value);
		}
	}

	for (const std::unique_ptr<StatementNode> &statement : node.statements) {
		if (statement->type == GdmlNodeType::NODE_DEFINITION) {
			const NodeDefinitionNode *child = static_cast<const NodeDefinitionNode *>(statement.get());
			Variant child_variant = visitNodeDefinition(*child);

			if (child_variant.get_type() != Variant::Type::NIL) {
				Object *child_obj = child_variant.get_validated_object();
				assert(child_obj);
				Node *child_node = Object::cast_to<Node>(child_obj);
				assert(child_node);
				obj_as_node->add_child(child_node);
			}
		} else if (statement->type == GdmlNodeType::FOR_STATEMENT) {
			ForStatementNode *for_node = static_cast<ForStatementNode *>(statement.get());
			std::vector<Variant> nodes;
			visitForStatement(*for_node, nodes);

			for (Variant &item : nodes) {
				obj_as_node->add_child(Object::cast_to<Node>(item));
			}
		}
	}

	return root_variant;
}

void TreeRendererVisitor::visitForStatement(ForStatementNode &statement, std::vector<Variant> &out_nodes) {
	Variant range_variant;
	std::shared_ptr<std::vector<godot::Variant>> nodes = std::make_shared<std::vector<Variant>>();
	String iter_name;

	if (statement.iter_name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
		iter_name = static_cast<IdentifierNode &>(*statement.iter_name).name;
	} else {
		print_error("Cannot use non-identifier expressions in for-statement iterator");
		return;
	}
	
	if (resolveExpression(range_variant, *statement.range, range_variant)) {
		if (range_variant.get_type() == Variant::ARRAY) {
			Array arr = range_variant;
			for (Variant item : arr) {
				m_context->set_data(iter_name, item);
				for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
					m_context->push_active_context();
					Variant result_node = visitNodeDefinition(*item);
					m_context->pop_active_context();
					out_nodes.push_back(result_node);
					nodes->push_back(result_node);
				}
			}
			m_context->set_data(iter_name, Variant());
		} else {
			for (int32_t i = 0; i < (int32_t)range_variant; i++) {
				m_context->set_data(iter_name, i);
				for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
					m_context->push_active_context();
					Variant result_node = visitNodeDefinition(*item);
					m_context->pop_active_context();
					out_nodes.push_back(result_node);
					nodes->push_back(result_node);
				}
			}
			m_context->set_data(iter_name, Variant());
		}
	} else {
		print_error("Couldn't resolve range expression of for-statement");
		return;
	}

	std::function<void()> render_callback = [nodes, &statement, this, iter_name]() {
		if (nodes->size() == 0) {
			return;
		}

		Node* parent = Object::cast_to<Node>((*nodes)[0])->get_parent();

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

		Variant range_variant;
		if (resolveExpression(range_variant, *statement.range, range_variant)) {
			if (range_variant.get_type() == Variant::ARRAY) {
				Array arr = range_variant;
				for (Variant item : arr) {
					m_context->set_data(iter_name, item);
					for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
						m_context->push_active_context();
						Variant result_node = visitNodeDefinition(*item);
						m_context->pop_active_context();

						Node *node = Object::cast_to<Node>(result_node);
						if (node) {
							parent->add_child(node);
							node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
							nodes->push_back(result_node);
						}
					}
				}
				m_context->set_data(iter_name, Variant());
			} else {
				for (int32_t i = 0; i < (int32_t)range_variant; i++) {
					m_context->set_data(iter_name, i);
					for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
						m_context->push_active_context();
						Variant result_node = visitNodeDefinition(*item);
						m_context->pop_active_context();

						Node *node = Object::cast_to<Node>(result_node);
						if (node) {
							parent->add_child(node);
							node->set_owner(parent->get_owner() ? parent->get_owner() : parent);
							nodes->push_back(result_node);
						}
					}
				}
				m_context->set_data(iter_name, Variant());
			}
		} else {
			print_error("Couldn't resolve range expression of for-statement");
		}

		for (std::pair<String, traversal::TreeRendererVisitorContext::SignalTarget> pair : m_context->m_signals) {
			std::vector<godot::ObjectID> to_remove;

			for (std::pair<godot::ObjectID, std::set<StringName>> target_pair : pair.second) {
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
	};

	for (Ref<reactive::GDMLAtom> atom : m_context->get_active_variables()) {
		atom->add_callback(render_callback);
	}
}

void TreeRendererVisitor::visitSignalDefinition(godot::SignalDefinitionNode &definition) {
	m_context->m_signals[definition.name->name] = TreeRendererVisitorContext::SignalTarget();
}

bool TreeRendererVisitor::setVariantProperty(Variant &root_variant, Variant &variant, String &prop_key, const ExpressionNode &value_node) {
	bool is_valid = false;

	if (value_node.type == GdmlNodeType::STRING_EXPRESSION) {
		const StringNode &node = (StringNode &)value_node;
		variant.set(prop_key, node.value, &is_valid);
	} else if (value_node.type == GdmlNodeType::NUMBER_EXPRESSION) {
		NumberNode node = (NumberNode &)value_node;
		if (node.isFloat) {
			variant.set(prop_key, node.getFloat(), &is_valid);
		} else {
			variant.set(prop_key, node.getInteger(), &is_valid);
		}
	} else if (value_node.type == GdmlNodeType::BOOLEAN_EXPRESSION) {
		const BooleanNode &node = (BooleanNode &)value_node;
		variant.set(prop_key, node.value, &is_valid);
	} else if (value_node.type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
		IdentifierNode &node = (IdentifierNode &)value_node;

		if (m_context->has_variable(node.name)) {
			subscribeToVariable(node.name, prop_key, variant);
		}

		Variant out_value;
		if (getPropertyValue(root_variant, node.name, out_value)) {
			variant.set(prop_key, out_value, &is_valid);
		} else {
			print_error("Couldn't access property \"" + node.name + "\" to get value");
		}
	} else if (value_node.type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
		const MemberExpressionNode &node = (MemberExpressionNode &)value_node;
		Variant out_value;
		if (getDeepPropertyValue(root_variant, node.path, out_value)) {
			variant.set(prop_key, out_value, &is_valid);
		} else {
			PackedStringArray arr;
			for (const std::unique_ptr<IdentifierNode> &idendifier : node.path) {
				arr.append(idendifier->name);
			}
			print_error("Couldn't access property by path \"" + String(".").join(arr) + "\" to get value");
		}
	} else if (value_node.type == GdmlNodeType::RESOURCE_DEFINITION) {
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
			if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				const MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, resource_variant, member_access->path, *property->value.get());
			}
		}

		variant.set(prop_key, resource_variant, &is_valid);
	} else if (value_node.type == GdmlNodeType::FUNCTION_CALL) {
		const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)value_node;

		if (node.name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
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
					variant.set(prop_key, result, &is_valid);
				} else {
					print_error("Cannot call non-Callable");
				}
			}
		} else if (node.name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
			MemberExpressionNode *name_node = static_cast<MemberExpressionNode *>(node.name.get());

			if (name_node->path.size() < 2) {
				print_error("Invalid member access expression");
				return false;
			}

			String method_name = name_node->path.back()->name;
			Variant current_variant = root_variant;

			for (size_t i = 0; i < name_node->path.size() - 1; i++) {
				String &segment_name = name_node->path[i]->name;
				bool is_valid = false;

				// Check local variables inside statements
				if (m_context->has_data(segment_name)) {
					current_variant = m_context->get_data(segment_name);
					continue;
				}

				// We have a value go check next
				Variant next_value = current_variant.get(segment_name, &is_valid);

				if (is_valid) {
					current_variant = next_value;
					continue;
				}
			}

			if (current_variant.has_method(method_name)) {
				Callable callable = current_variant.get(method_name);
				Array args;
	
				for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
					Variant arg_value;
					if (resolveExpression(root_variant, *expr.get(), arg_value)) {
						args.push_back(arg_value);
					} else {
						print_error("Unsupported argument of type " + String::num_int64((int)expr->type) + " in function \"" + name_node->path.back()->name + "\"");
					}
				}
	
				const Variant result = callable.callv(args);
				variant.set(prop_key, result, &is_valid);
			}
		} else {
			print_error("Other types of expressions are not implemented for a function call");
		}
	}

	return is_valid;
}

bool TreeRendererVisitor::setVariantPropertyDeep(Variant &root_variant, Variant &variant, const std::vector<std::unique_ptr<IdentifierNode>> &path, const ExpressionNode &value_node) {
	Variant previous_variant;
	Variant current_variant = variant;

	bool is_valid = false;
	for (auto it = path.begin(); it != path.end() - 1; ++it) {
		previous_variant = current_variant;
		current_variant = current_variant.get(it->get()->name, &is_valid);

		if (!is_valid) {
			print_error("Cannot get property " + it->get()->name + " in path");
			return false;
		}
	}

	String &variant_property = path.back()->name;
	if (setVariantProperty(root_variant, current_variant, variant_property, value_node)) {
		const String &previous_property = (path.end() - 2)->get()->name;
		previous_variant.set(previous_property, current_variant, &is_valid);
	}

	return is_valid;
}

bool TreeRendererVisitor::getPropertyValue(Variant &variant, String &prop_key, Variant &value) {
	/*
		1. context prop
		2. object prop
		3. static object prop
		4. object constant prop
	*/

	bool is_valid = false;

	if (!is_valid) {
		if (m_context->has_data(prop_key)) {
			value = m_context->get_data(prop_key);
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

	return is_valid;
}

bool TreeRendererVisitor::getDeepPropertyValue(Variant &root_variant, const std::vector<std::unique_ptr<IdentifierNode>> &path, Variant &value) {
	if (path.empty()) {
		return false;
	}
	
	// Variant previous_variant;
	Variant current_variant = root_variant;

	for (size_t i = 0; i < path.size(); i++) {
		String &segment_name = path[i]->name;
		bool is_valid = false;

		if (i == 0) {
			if (segment_name == "self") {
				continue;
			} else {
				// Check local variables inside statements
				if (m_context->has_data(segment_name)) {
					current_variant = m_context->get_data(segment_name);
					continue;
				}
			}
		}
		
		// We have a value go check next
		Variant next_value = current_variant.get(segment_name, &is_valid);

		if (is_valid) {
			current_variant = next_value;
			continue;
		}

		// Check if it's a constant of a basic Variant type
		if (m_variant_type_map.count(segment_name)) {
			if (i + 1 < path.size()) {
				const StringName value_name = path[++i]->name;
				Variant ret;
				internal::gdextension_interface_variant_get_constant_value(
						(GDExtensionVariantType)m_variant_type_map[segment_name],
						value_name._native_ptr(),
						&ret);
				if (ret.get_type() != Variant::NIL) {
					current_variant = ret;
					continue;
				}
			}
		}

		// Check if it's a class and has the next path segment as a constant
		if (ClassDB::class_exists(segment_name)) {
			if (i + 1 < path.size()) {
				const StringName const_name = path[i + 1]->name;
	
				if (ClassDB::class_has_integer_constant(segment_name, const_name)) {
					current_variant = ClassDB::class_get_integer_constant(segment_name, const_name);
					i++;
					continue;
				}
			}
		}

		Object *obj = current_variant.get_validated_object();
		if (obj) {
			const String class_name = obj->get_class();

			if (ClassDB::class_has_integer_constant(class_name, segment_name)) {
				current_variant = ClassDB::class_get_integer_constant(class_name, segment_name);
				continue;
			}
		}

		return false;
	}

	value = current_variant;
	return true;
}

void TreeRendererVisitor::setControlThemeOverride(godot::Variant &root_variant, godot::Variant &variant, godot::String &prop_key, const godot::ExpressionNode &value_node) {
	Control *control = Object::cast_to<Control>(variant);

	// Ref<godot::Theme> control_theme = control->get_theme();
	// StringName theme_type = control->get_theme_type_variation();

	// if (control_theme.is_null()) {
	// 	control_theme = ThemeDB::get_singleton()->get_project_theme();
		
	// }

	// if (control_theme.is_null()) {
	// 	control_theme = ThemeDB::get_singleton()->get_default_theme();
	// }

	if (value_node.type == GdmlNodeType::NUMBER_EXPRESSION) {
		NumberNode node = (NumberNode &)value_node;

		if (control->has_theme_constant(prop_key)) {
			control->add_theme_constant_override(prop_key, node.getFloat());
		} else if (control->has_theme_font_size(prop_key)) {
			control->add_theme_font_size_override(prop_key, node.getFloat());
		}
	} else if (value_node.type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
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
	} else if (value_node.type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
		const MemberExpressionNode &node = (MemberExpressionNode &)value_node;
		Variant out_value;
		if (getDeepPropertyValue(root_variant, node.path, out_value)) {
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
			PackedStringArray arr;
			for (const std::unique_ptr<IdentifierNode> &idendifier : node.path) {
				arr.append(idendifier->name);
			}
			print_error("Couldn't access property by path \"" + String(".").join(arr) + "\" to get value");
		}
	} else if (value_node.type == GdmlNodeType::RESOURCE_DEFINITION) {
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
			if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
				IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
				setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				const MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, resource_variant, member_access->path, *property->value.get());
			}
		}

		if (control->has_theme_icon(prop_key)) {
			control->add_theme_icon_override(prop_key, resource_variant);
		} else if (control->has_theme_stylebox(prop_key)) {
			control->add_theme_stylebox_override(prop_key, resource_variant);
		} else if (control->has_theme_font(prop_key)) {
			control->add_theme_font_override(prop_key, resource_variant);
		}
	} else if (value_node.type == GdmlNodeType::FUNCTION_CALL) {
		const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)value_node;

		if (node.name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
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

bool TreeRendererVisitor::connectSignal(Variant &root_variant, Variant &variant, const String &signal_name, const String &callable_name) {
	Object *obj = variant.get_validated_object();
	
	if (m_context->m_signals.find(callable_name) != m_context->m_signals.end()) {

		if (m_context->m_signals[callable_name].find((ObjectID)obj->get_instance_id()) == m_context->m_signals[callable_name].end()) {
			m_context->m_signals[callable_name][(ObjectID)obj->get_instance_id()] = std::set<StringName>();
		}

		m_context->m_signals[callable_name][(ObjectID)obj->get_instance_id()].insert(signal_name);
		return true;
	}

	if (obj) {
		const Variant name_or_callable = m_context->m_data[callable_name];
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
		case GdmlNodeType::STRING_EXPRESSION: {
			StringNode &node = static_cast<StringNode &>(expr_node);
			value = node.value;
		} break;
		case GdmlNodeType::NUMBER_EXPRESSION: {
			NumberNode node = static_cast<NumberNode &>(expr_node);
			value = node.isFloat ? node.getFloat() : node.getInteger();
		} break;
		case GdmlNodeType::BOOLEAN_EXPRESSION: {
			BooleanNode &node = static_cast<BooleanNode &>(expr_node);
			value = node.value;
		} break;
		case GdmlNodeType::IDENTIFIER_EXPRESSION: {
			IdentifierNode &node = static_cast<IdentifierNode &>(expr_node);
			Variant out_value;
			if (getPropertyValue(root_variant, node.name, out_value)) {
				value = out_value;
			} else {
				print_error("Couldn't access property \"" + node.name + "\" to get value");
				return false;
			}
		} break;
		case GdmlNodeType::ARRAY_EXPRESSION: {
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
		case GdmlNodeType::DICTIONARY_EXPRESSION: {
			DictionaryExpressionNode &node = static_cast<DictionaryExpressionNode &>(expr_node);
			Dictionary out_dict;

			for (const std::unique_ptr<PropertyDefinitionNode> &item_node : node.values) {
				if (item_node->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
					String &name = static_cast<IdentifierNode *>(item_node->name.get())->name;
					Variant out_value;
					if (resolveExpression(root_variant, *item_node->value.get(), out_value)) {
						out_dict[name] = out_value;
					}
				}
			}

			value = out_dict;
		} break;
		case GdmlNodeType::MEMBER_ACCESS_EXPRESSION: {
			MemberExpressionNode &node = static_cast<MemberExpressionNode &>(expr_node);
			Variant out_value;
			if (getDeepPropertyValue(root_variant, node.path, out_value)) {
				value = out_value;
			} else {
				PackedStringArray arr;
				for (const std::unique_ptr<IdentifierNode> &idendifier : node.path) {
					arr.append(idendifier->name);
				}
				print_error("Couldn't access property by path \"" + String(".").join(arr) + "\" to get value");
				return false;
			}
		} break;
		case GdmlNodeType::RESOURCE_DEFINITION: {
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
				if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
					IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
					setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
				} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
					const MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
					setVariantPropertyDeep(root_variant, resource_variant, member_access->path, *property->value.get());
				}
			}

			value = resource_variant;
		} break;
		case GdmlNodeType::FUNCTION_CALL: {
			const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)expr_node;

			// if (node.name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
			// 	IdentifierNode *name_node = static_cast<IdentifierNode *>(node.name.get());

			// 	if (m_context->has_variable(name_node->name)) {
			// 		Ref<reactive::GDMLAtom> atom = m_context->get_variable(name_node->name);
			// 		Variant value = atom->get_value();

			// 		if (value.get_type() == Variant::Type::CALLABLE) {
			// 			Callable callable = value;
			// 			Array args;

			// 			for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
			// 				Variant arg_value;
			// 				if (resolveExpression(root_variant, *expr.get(), arg_value)) {
			// 					args.push_back(arg_value);
			// 				} else {
			// 					print_error("Unsupported argument of type " + String::num_int64((int)expr->type) + " in function \"" + name_node->name + "\"");
			// 				}
			// 			}

			// 			const Variant result = callable.callv(args);
			// 			variant.set(prop_key, result, &is_valid);
			// 		} else {
			// 			print_error("Cannot call non-Callable");
			// 		}
			// 	}
			// } else 
			if (node.name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				MemberExpressionNode *name_node = static_cast<MemberExpressionNode *>(node.name.get());

				if (name_node->path.size() < 2) {
					print_error("Invalid member access expression");
					return false;
				}

				String method_name = name_node->path.back()->name;
				Variant current_variant = root_variant;

				for (size_t i = 0; i < name_node->path.size() - 1; i++) {
					String &segment_name = name_node->path[i]->name;
					bool is_valid = false;

					// Check local variables inside statements
					if (m_context->has_data(segment_name)) {
						current_variant = m_context->get_data(segment_name);
						continue;
					}

					// We have a value go check next
					Variant next_value = current_variant.get(segment_name, &is_valid);

					if (is_valid) {
						current_variant = next_value;
						continue;
					}
				}

				if (current_variant.has_method(method_name)) {
					Callable callable = current_variant.get(method_name);
					Array args;
		
					for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
						Variant arg_value;
						if (resolveExpression(root_variant, *expr.get(), arg_value)) {
							args.push_back(arg_value);
						} else {
							print_error("Unsupported argument of type " + String::num_int64((int)expr->type) + " in function \"" + name_node->path.back()->name + "\"");
						}
					}

					value = callable.callv(args);
				}
			} else {
				print_error("Other types of expressions are not implemented for a function call");
			}
		} break;
		default: {
			print_error("Cannot resolve unknown expression type");
			return false;
		}
	}

	return true;
}

void TreeRendererVisitor::subscribeToVariable(godot::String &p_variable, godot::String &p_prop, godot::Variant &variant) {
	Ref<reactive::GDMLAtom> atom = m_context->get_variable(p_variable);
	atom->add_dependency(variant, p_prop);
}

void TreeRendererVisitor::printError(const godot::String p_error, const godot::GdmlNode &node) {
	print_error(p_error, " at ", node.location.start.line, ":", node.location.start.column);
}

#pragma endregion

}

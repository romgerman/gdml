#include "tree_renderer_visitor.h"

#include <assert.h>

#include <godot_cpp/classes/node.hpp>
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

void TreeRendererVisitorContext::set_variable(String &p_name, Variant p_value) {
	if (m_variables.has(p_name)) {
		Ref<reactive::GDMLAtom> atom = m_variables[p_name];
		atom->set_value(p_value);
	} else {
		Ref<reactive::GDMLAtom> atom = Ref<reactive::GDMLAtom>(memnew(reactive::GDMLAtom()));
		atom->create(p_name, p_value);
		m_variables[p_name] = atom;
	}
}

Ref<reactive::GDMLAtom> TreeRendererVisitorContext::get_variable(String &p_name) {
	if (m_variables.has(p_name)) {
		return m_variables[p_name];
	} else {
		return nullptr;
	}
}

bool TreeRendererVisitorContext::has_variable(String &p_name) {
	return m_variables.has(p_name);
}

TreeRendererVisitor::TreeRendererVisitor() {
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
		visitSourceFile((SourceFileNode &)root_node, result);
		result.variables = m_context->m_variables;
		return result;
	} else {
		print_error("Cannot render tree. The tree is malformed");
		return TreeRendererVisitorResult();
	}
}

void TreeRendererVisitor::visitSourceFile(const SourceFileNode &source_node, TreeRendererVisitorResult &result) {
	for (const std::unique_ptr<DefinitionNode> &definition : source_node.statements) {
		if (definition->statement->type == GdmlNodeType::NODE_DEFINITION) {
			const NodeDefinitionNode *node_def = static_cast<const NodeDefinitionNode *>(definition->statement.get());
			Variant result_node = visitNodeDefinition(*node_def);
			result.nodes.push_back(result_node);
		} else if (definition->statement->type == GdmlNodeType::VARIABLE_DEFINITION) {
			Variant value;
			VariableDefinitionNode *var = static_cast<VariableDefinitionNode *>(definition->statement.get());

			if (resolveExpression(value, *var->value, value)) {
				m_context->set_variable(var->name->name, value);
			} else {
				print_error("Couldn't resolve variable expression");
			}
		} else if (definition->statement->type == GdmlNodeType::FOR_STATEMENT) {
			ForStatementNode *statement = static_cast<ForStatementNode *>(definition->statement.get());
			visitForStatement(*statement, result.nodes);
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
			print_error("Class with name " + node.name->name + " is not found");
			return nullptr;
		}

		String path = class_info.get("path", "");
		Ref<Script> script = ResourceLoader::get_singleton()->load(path, "Script");
		if (script.is_null()) {
			print_error("Could not load script associated with type " + node.name->name + ". Path: " + path);
			return nullptr;
		}
		root_variant = ClassDB::instantiate(script->get_instance_base_type());
		Object *base_object = root_variant.get_validated_object();
		assert(base_object);
		base_object->set_script(script);
	} else {
		if (!ClassDB::can_instantiate(node.name->name)) {
			print_error("Cannot construct node of type " + node.name->name);
			return nullptr;
		}

		root_variant = ClassDB::instantiate(node.name->name);
	}

	for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
		if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
			const IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
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
					print_error("Cannot connect signal. Callable name should be an identifier or a string");
				}
			} else {
				print_error("Cannot connect signal. Wrong name expression type");
			}
		}
	}

	for (const std::unique_ptr<VariableDefinitionNode> &var : node.variables) {
		Variant value;

		if (resolveExpression(value, *var, value)) {
			m_context->set_variable(var->name->name, value);
		}
	}

	bool is_node = false;

	if (root_variant.get_type() == Variant::Type::OBJECT) {
		Object *obj = root_variant.get_validated_object();
		assert(obj);
		is_node = obj->is_class("Node");
	}

	if (is_node) {
		Object *obj = root_variant.get_validated_object();
		assert(obj);
		Node *obj_as_node = Object::cast_to<Node>(obj);
		assert(obj_as_node);
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
	} else {
		print_error("Cannot have children on non-Node class");
	}

	return root_variant;
}

void TreeRendererVisitor::visitForStatement(ForStatementNode &statement, std::vector<Variant> &out_nodes) {
	Variant range_variant;

	if (resolveExpression(range_variant, *statement.range, range_variant)) {
		for (int32_t i = 0; i < (int32_t)range_variant; i++) {
			for (std::unique_ptr<NodeDefinitionNode> &item : statement.statements) {
				Variant result_node = visitNodeDefinition(*item);
				out_nodes.push_back(result_node);
			}
		}
	} else {
		print_error("Couldn't resolve range expression of the for-statement");
	}
}

bool TreeRendererVisitor::setVariantProperty(Variant &root_variant, Variant &variant, const String &prop_key, const ExpressionNode &value_node) {
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
				const IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
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

	const String &variant_property = path.back()->name;
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
		if (m_context->has_variable(prop_key)) {
			Ref<reactive::GDMLAtom> atom = m_context->get_variable(prop_key);
			value = atom->get_value();
			is_valid = true;
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

bool TreeRendererVisitor::getDeepPropertyValue(Variant &variant, const std::vector<std::unique_ptr<IdentifierNode>> &path, Variant &value) {
	Variant previous_variant;
	Variant current_variant = variant;

	bool is_valid = false;
	for (auto it = path.begin(); it != path.end() - 1; ++it) {
		previous_variant = current_variant;
		current_variant = current_variant.get(it->get()->name, &is_valid);

		if (!is_valid) {
			const String class_name = variant.get_validated_object()->get_class();
			if (ClassDB::class_has_enum(class_name, it->get()->name)) {
				if (it == path.end()) {
					PackedStringArray arr;
					for (const std::unique_ptr<IdentifierNode> &idendifier : path) {
						arr.append(idendifier->name);
					}
					print_error("No path parts left. Cannot get enum constant in path \"" + String(".").join(arr) + "\"");
					return false;
				}

				const String &enum_const_name = (++it)->get()->name;
				const int64_t enum_value = ClassDB::class_get_integer_constant(class_name, enum_const_name);
				value = enum_value;
				return true;
			}

			print_error("Cannot get property " + it->get()->name + " in path");
			return false;
		}
	}

	if (is_valid) {
		value = current_variant;
	}

	return is_valid;
}

bool TreeRendererVisitor::connectSignal(Variant &root_variant, Variant &variant, const String &signal_name, const String &callable_name) {
	Object *obj = variant.get_validated_object();

	if (obj) {
		const Variant name_or_callable = m_context->m_data.get(callable_name, Variant(nullptr));
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
					const IdentifierNode *identifier = static_cast<IdentifierNode *>(property->name.get());
					setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
				} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
					const MemberExpressionNode *member_access = static_cast<MemberExpressionNode *>(property->name.get());
					setVariantPropertyDeep(root_variant, resource_variant, member_access->path, *property->value.get());
				}
			}

			value = resource_variant;
		} break;
		default: {
			print_error("Cannot resolve unknown expression type");
			return false;
		}
	}

	return true;
}

}

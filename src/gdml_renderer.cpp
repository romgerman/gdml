#include "gdml_renderer.h"

#include <assert.h>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;
using namespace parser;

// Visitor

traversal::TreeRendererVisitor::TreeRendererVisitor() {
	m_variant_type_map = {
		{ "Vector2", godot::Variant::Type::VECTOR2 },
		{ "Vector2i", godot::Variant::Type::VECTOR2I },
		{ "Rect2", godot::Variant::Type::RECT2 },
		{ "Rect2i", godot::Variant::Type::RECT2I },
		{ "Vector3", godot::Variant::Type::VECTOR3 },
		{ "Vector3i", godot::Variant::Type::VECTOR3I },
		{ "Transform2D", godot::Variant::Type::TRANSFORM2D },
		{ "Vector4", godot::Variant::Type::VECTOR4 },
		{ "Vector4i", godot::Variant::Type::VECTOR4I },
		{ "Plane", godot::Variant::Type::PLANE },
		{ "Quaternion", godot::Variant::Type::QUATERNION },
		{ "AABB", godot::Variant::Type::AABB },
		{ "Basis", godot::Variant::Type::BASIS },
		{ "Transform3D", godot::Variant::Type::TRANSFORM3D },
		{ "Projection", godot::Variant::Type::PROJECTION },

		{ "Color", godot::Variant::Type::COLOR },
		{ "StringName", godot::Variant::Type::STRING_NAME },
		{ "NodePath", godot::Variant::Type::NODE_PATH },
		{ "RID", godot::Variant::Type::RID },
		{ "CALLABLE", godot::Variant::Type::CALLABLE },
		{ "Signal", godot::Variant::Type::SIGNAL },
		{ "Dictionary", godot::Variant::Type::DICTIONARY },
		{ "Array", godot::Variant::Type::ARRAY },

		{ "PackedByteArray", godot::Variant::Type::PACKED_BYTE_ARRAY },
		{ "PackedInt32Array", godot::Variant::Type::PACKED_INT32_ARRAY },
		{ "PackedInt64Array", godot::Variant::Type::PACKED_INT64_ARRAY },
		{ "PackedFloat32Array", godot::Variant::Type::PACKED_FLOAT32_ARRAY },
		{ "PackedFloat64Array", godot::Variant::Type::PACKED_FLOAT64_ARRAY },
		{ "PackedStringArray", godot::Variant::Type::PACKED_STRING_ARRAY },
		{ "PackedVector2Array", godot::Variant::Type::PACKED_VECTOR2_ARRAY },
		{ "PackedVector3Array", godot::Variant::Type::PACKED_VECTOR3_ARRAY },
		{ "PackedColorArray", godot::Variant::Type::PACKED_COLOR_ARRAY },
		{ "PackedVector4Array", godot::Variant::Type::PACKED_VECTOR4_ARRAY },
	};
}

godot::Variant traversal::TreeRendererVisitor::visit(const godot::GdmlNode &root_node) {
	if (root_node.type == GdmlNodeType::SOURCE_FILE) {
		return visitSourceFile((godot::SourceFileNode &)root_node);
	} else {
		print_error("Cannot render tree. The tree is malformed");
		return nullptr;
	}
}

void traversal::TreeRendererVisitor::setRendererData(const godot::Dictionary &data) {
	m_context = std::make_unique<TreeRendererVisitorContext>();
	m_context->data = data;
}

godot::Variant traversal::TreeRendererVisitor::visitSourceFile(const godot::SourceFileNode &node) {
	for (const std::unique_ptr<godot::DefinitionNode> &statement : node.statements) {
		if (statement->node && statement->node->type == godot::GdmlNodeType::NODE_DEFINITION) {
			// Render first one
			return visitNodeDefinition(*statement->node.get());
		}
	}

	// No nodes in tree
	return nullptr;
}

godot::Variant traversal::TreeRendererVisitor::visitNodeDefinition(const godot::NodeDefinitionNode &node) {
	if (!ClassDB::can_instantiate(node.name->name)) {
		print_error("Cannot construct node of type " + node.name->name);
		return nullptr;
	}

	godot::Variant root_variant;

	if (!ClassDB::class_exists(node.name->name)) {
		godot::Dictionary class_info;

		for (const godot::Dictionary &ci : godot::ProjectSettings::get_singleton()->get_global_class_list()) {
			if (String(ci.get("class", "")) == node.name->name) {
				class_info = ci;
				break;
			}
		}

		if (class_info.is_empty()) {
			print_error("Class with name " + node.name->name + " is not found");
			return nullptr;
		}

		godot::String path = class_info.get("path", "");
		godot::Ref<Script> script = godot::ResourceLoader::get_singleton()->load(path, "Script");
		if (script.is_null()) {
			print_error("Could not load script associated with type " + node.name->name + ". Path: " + path);
			return nullptr;
		}
		root_variant = ClassDB::instantiate(script->get_instance_base_type());
		godot::Object *base_object = root_variant.get_validated_object();
		assert(base_object);
		base_object->set_script(script);
	} else {
		root_variant = ClassDB::instantiate(node.name->name);
	}

	for (const std::unique_ptr<godot::PropertyDefinitionNode> &property : node.properties) {
		if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
			const godot::IdentifierNode *identifier = static_cast<godot::IdentifierNode *>(property->name.get());
			setVariantProperty(root_variant, root_variant, identifier->name, *property->value.get());
		} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
			const godot::MemberExpressionNode *member_access = static_cast<godot::MemberExpressionNode *>(property->name.get());
			setVariantPropertyDeep(root_variant, root_variant, member_access->path, *property->value.get());
		} else if (property->name->type == GdmlNodeType::SIGNAL_EXPRESSION) {
			const godot::SignalExpressionNode *signal_expression = static_cast<godot::SignalExpressionNode *>(property->name.get());
			if (signal_expression->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
				const godot::String signal_name = static_cast<godot::IdentifierNode *>(signal_expression->name.get())->name;
				if (property->value->type == GdmlNodeType::IDENTIFIER_EXPRESSION || property->value->type == GdmlNodeType::STRING_EXPRESSION) {
					if (property->value->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<godot::IdentifierNode *>(property->value.get())->name);
					} else if (property->value->type == GdmlNodeType::STRING_EXPRESSION) {
						connectSignal(root_variant, root_variant, signal_name, static_cast<godot::StringNode *>(property->value.get())->value);
					}
				} else {
					print_error("Cannot connect signal. Callable name should be an identifier or a string");
				}
			} else {
				print_error("Cannot connect signal. Wrong name expression type");
			}
		}
	}

	bool is_node = false;

	if (root_variant.get_type() == godot::Variant::Type::OBJECT) {
		godot::Object *obj = root_variant.get_validated_object();
		assert(obj);
		is_node = obj->is_class("Node");
	}

	if (is_node) {
		godot::Object *obj = root_variant.get_validated_object();
		assert(obj);
		godot::Node *obj_as_node = static_cast<godot::Node *>(obj);
		assert(obj_as_node);
		for (const std::unique_ptr<godot::NodeDefinitionNode> &child : node.children) {
			godot::Variant child_variant = visitNodeDefinition(*child.get());

			if (child_variant.get_type() != godot::Variant::Type::NIL) {
				godot::Object *child_obj = child_variant.get_validated_object();
				assert(child_obj);
				godot::Node *child_node = static_cast<godot::Node *>(child_obj);
				assert(child_node);
				obj_as_node->add_child(child_node);
			}
		}
	} else {
		print_error("Cannot have children on non-Node class");
	}

	return root_variant;
}

bool traversal::TreeRendererVisitor::setVariantProperty(godot::Variant &root_variant, godot::Variant &variant, const godot::String &prop_key, const godot::ExpressionNode &value_node) {
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
		const IdentifierNode &node = (IdentifierNode &)value_node;
		godot::Variant out_value;
		if (getPropertyValue(root_variant, node.name, out_value)) {
			variant.set(prop_key, out_value, &is_valid);
		} else {
			print_error("Couldn't access property \"" + node.name + "\" to get value");
		}
	} else if (value_node.type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
		const MemberExpressionNode &node = (MemberExpressionNode &)value_node;
		godot::Variant out_value;
		if (getDeepPropertyValue(root_variant, node.path, out_value)) {
			variant.set(prop_key, out_value, &is_valid);
		} else {
			godot::PackedStringArray arr;
			for (const std::unique_ptr<godot::IdentifierNode> &idendifier : node.path) {
				arr.append(idendifier->name);
			}
			print_error("Couldn't access property by path \"" + String(".").join(arr) + "\" to get value");
		}
	} else if (value_node.type == GdmlNodeType::RESOURCE_DEFINITION) {
		const ResourceDefinitionNode &node = (ResourceDefinitionNode &)value_node;
		godot::Variant resource_variant;

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
				const godot::IdentifierNode *identifier = static_cast<godot::IdentifierNode *>(property->name.get());
				setVariantProperty(root_variant, resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				const godot::MemberExpressionNode *member_access = static_cast<godot::MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(root_variant, resource_variant, member_access->path, *property->value.get());
			}
		}

		variant.set(prop_key, resource_variant, &is_valid);
	} else if (value_node.type == GdmlNodeType::FUNCTION_CALL) {
		const FunctionCallExpressionNode &node = (FunctionCallExpressionNode &)value_node;

		if (node.name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
			const IdentifierNode *name_node = static_cast<IdentifierNode *>(node.name.get());

			if (m_context->data.has(name_node->name)) {
				godot::Variant value = m_context->data.get(name_node->name, godot::Variant(nullptr));

				if (value.get_type() == godot::Variant::Type::CALLABLE) {
					godot::Callable callable = value;
					godot::Array args;

					for (const std::unique_ptr<ExpressionNode> &expr : node.arguments) {
						godot::Variant arg_value;
						if (resolveExpression(root_variant, *expr.get(), arg_value)) {
							args.push_back(arg_value);
						} else {
							print_error("Unsupported argument of type " + String::num_int64((int)expr->type) + " in function \"" + name_node->name + "\"");
						}
					}

					const godot::Variant result = callable.callv(args);
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

bool traversal::TreeRendererVisitor::setVariantPropertyDeep(godot::Variant &root_variant, godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, const godot::ExpressionNode &value_node) {
	godot::Variant previous_variant;
	godot::Variant current_variant = variant;

	bool is_valid = false;
	for (auto it = path.begin(); it != path.end() - 1; ++it) {
		previous_variant = current_variant;
		current_variant = current_variant.get(it->get()->name, &is_valid);

		if (!is_valid) {
			print_error("Cannot get property " + it->get()->name + " in path");
			return false;
		}
	}

	const godot::String &variant_property = path.back()->name;
	if (setVariantProperty(root_variant, current_variant, variant_property, value_node)) {
		const godot::String &previous_property = (path.end() - 2)->get()->name;
		previous_variant.set(previous_property, current_variant, &is_valid);
	}

	return is_valid;
}

bool traversal::TreeRendererVisitor::getPropertyValue(godot::Variant &variant, const godot::String &prop_key, godot::Variant &value) {
	/*
		1. context prop
		2. object prop
		3. static object prop
		4. object constant prop
	*/

	bool is_valid = false;

	if (!is_valid) {
		if (m_context->data.has(prop_key)) {
			value = m_context->data.get(prop_key, godot::Variant(nullptr));
			is_valid = true;
		}
	}

	if (!is_valid) {
		const godot::Variant prop_value = variant.get(prop_key, &is_valid);
		if (is_valid) {
			value = prop_value;
		}
	}

	return is_valid;
}

bool traversal::TreeRendererVisitor::getDeepPropertyValue(godot::Variant &variant, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, godot::Variant &value) {
	godot::Variant previous_variant;
	godot::Variant current_variant = variant;

	bool is_valid = false;
	for (auto it = path.begin(); it != path.end() - 1; ++it) {
		previous_variant = current_variant;
		current_variant = current_variant.get(it->get()->name, &is_valid);

		if (!is_valid) {
			const godot::String class_name = variant.get_validated_object()->get_class();
			if (ClassDB::class_has_enum(class_name, it->get()->name)) {
				if (it == path.end()) {
					godot::PackedStringArray arr;
					for (const std::unique_ptr<godot::IdentifierNode> &idendifier : path) {
						arr.append(idendifier->name);
					}
					print_error("No path parts left. Cannot get enum constant in path \"" + String(".").join(arr) + "\"");
					return false;
				}

				const godot::String &enum_const_name = (++it)->get()->name;
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

bool traversal::TreeRendererVisitor::connectSignal(godot::Variant &root_variant, godot::Variant &variant, const godot::String &signal_name, const godot::String &callable_name) {
	godot::Object *obj = variant.get_validated_object();

	if (obj) {
		const godot::Variant name_or_callable = m_context->data.get(callable_name, Variant(nullptr));
		if (name_or_callable) {
			if (name_or_callable.get_type() == godot::Variant::STRING) {
				obj->connect(signal_name, godot::Callable(root_variant, name_or_callable));
			} else if (name_or_callable.get_type() == godot::Variant::CALLABLE) {
				obj->connect(signal_name, name_or_callable);
			} else {
				print_error("Trying to connect signal with \"" + name_or_callable.get_type_name(name_or_callable.get_type()) + "\"");
			}
		}
	} else {
		print_error("Trying to connect signal for non-Object type");
	}

	return false;
}

bool traversal::TreeRendererVisitor::resolveExpression(godot::Variant &root_variant, const ExpressionNode &expr_node, godot::Variant &value) {
	switch (expr_node.type) {
		case GdmlNodeType::STRING_EXPRESSION: {
			const StringNode &node = (StringNode &)expr_node;
			value = node.value;
		} break;
		case GdmlNodeType::NUMBER_EXPRESSION: {
			const NumberNode node = (NumberNode &)expr_node;
			value = node.isFloat ? node.getFloat() : node.getInteger();
		} break;
		case GdmlNodeType::BOOLEAN_EXPRESSION: {
			const BooleanNode &node = (BooleanNode &)expr_node;
			value = node.value;
		} break;
		case GdmlNodeType::IDENTIFIER_EXPRESSION: {
			const IdentifierNode &node = (IdentifierNode &)expr_node;
			godot::Variant out_value;
			if (getPropertyValue(root_variant, node.name, out_value)) {
				value = out_value;
			} else {
				print_error("Couldn't access property \"" + node.name + "\" to get value");
				return false;
			}
		} break;
		case GdmlNodeType::MEMBER_ACCESS_EXPRESSION: {
			const MemberExpressionNode &node = (MemberExpressionNode &)expr_node;
			godot::Variant out_value;
			if (getDeepPropertyValue(root_variant, node.path, out_value)) {
				value = out_value;
			} else {
				godot::PackedStringArray arr;
				for (const std::unique_ptr<godot::IdentifierNode> &idendifier : node.path) {
					arr.append(idendifier->name);
				}
				print_error("Couldn't access property by path \"" + String(".").join(arr) + "\" to get value");
				return false;
			}
		} break;
	}

	return true;
}

// Renderer

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
	return visitor.visit(*result.get());
}

Variant godot::GdmlRenderer::renderScript(GdmlScript *script, const godot::Dictionary &data) {
	return renderString(script->get_source_code(), data);
}

#include "gdml_renderer.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

// Visitor

Variant traversal::TreeRendererVisitor::visit(const godot::GdmlNode &root_node) {
	if (m_variant_type_map.size() == 0) {
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

	if (root_node.type == GdmlNodeType::SOURCE_FILE) {
		return visitSourceFile((godot::SourceFileNode &)root_node);
	} else {
		return nullptr;
	}
}

godot::Variant traversal::TreeRendererVisitor::visitSourceFile(const godot::SourceFileNode &node) {
	if (node.statements.size() > 1) {
		print_line("Currently doesn't support multiple nodes");
		return nullptr;
	}

	for (const std::unique_ptr<godot::DefinitionNode> &statement : node.statements) {
		if (!statement->node) {
			print_line("No node? WTF?");
			return nullptr;
		}

		return visitNodeDefinition(*statement->node.get());
	}

	print_line("Empty result");
	return nullptr;
}

godot::Variant traversal::TreeRendererVisitor::visitNodeDefinition(const godot::NodeDefinitionNode &node) {
	if (!ClassDB::can_instantiate(node.name->name)) {
		print_line("Cannot construct type " + node.name->name);
		return nullptr;
	}

	godot::Variant base_variant;

	if (!ClassDB::class_exists(node.name->name)) {
		godot::Dictionary c_info;

		for (const godot::Dictionary &cc : godot::ProjectSettings::get_singleton()->get_global_class_list()) {
			if (String(cc.get("class", "")) == node.name->name) {
				c_info = cc;
				break;
			}
		}

		if (c_info.is_empty()) {
			print_line("Class with name " + node.name->name + " is not found");
			return nullptr;
		}

		godot::String path = c_info.get("path", "");
		godot::Ref<Script> script = godot::ResourceLoader::get_singleton()->load(path, "Script");
		base_variant = ClassDB::instantiate(script->get_instance_base_type());
		godot::Object *base_object = base_variant.get_validated_object();
		base_object->set_script(script);
	} else {
		base_variant = ClassDB::instantiate(node.name->name);
	}

	for (const std::unique_ptr<godot::PropertyDefinitionNode> &property : node.properties) {
		if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
			const godot::IdentifierNode *identifier = static_cast<godot::IdentifierNode *>(property->name.get());
			setVariantProperty(base_variant, identifier->name, *property->value.get());
		} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
			const godot::MemberExpressionNode *member_access = static_cast<godot::MemberExpressionNode *>(property->name.get());
			setVariantPropertyDeep(base_variant, member_access->path, *property->value.get());
		}
	}

	bool is_node = false;

	if (base_variant.get_type() == godot::Variant::Type::OBJECT) {
		godot::Object *obj = static_cast<godot::Object *>(base_variant);

		if (obj->is_class("Node")) {
			is_node = true;
		}
	}

	if (is_node) {
		godot::Object *obj = static_cast<godot::Object *>(base_variant);
		godot::Node *as_node = static_cast<godot::Node *>(obj);
		for (const std::unique_ptr<godot::NodeDefinitionNode> &child : node.children) {
			godot::Variant child_control = visitNodeDefinition(*child.get());

			if (child_control.get_type() == godot::Variant::Type::NIL) {
				continue;
			}

			godot::Object *child_obj = static_cast<godot::Object *>(child_control);
			godot::Node *child_node = static_cast<godot::Node *>(child_obj);
			as_node->add_child(child_node);
		}
	} else {
		print_line("Cannot have children on non-Node class");
	}

	return base_variant;
}

bool traversal::TreeRendererVisitor::setVariantProperty(godot::Variant &obj, const godot::String &prop_key, const godot::ExpressionNode &value_node) {
	bool is_valid = false;

	if (value_node.type == GdmlNodeType::STRING_EXPRESSION) {
		const StringNode &node = (StringNode &)value_node;
		obj.set(prop_key, node.value, &is_valid);
	} else if (value_node.type == GdmlNodeType::NUMBER_EXPRESSION) {
		NumberNode node = (NumberNode &)value_node;
		if (node.isFloat) {
			obj.set(prop_key, node.getFloat(), &is_valid);
		} else {
			obj.set(prop_key, node.getInteger(), &is_valid);
		}
	} else if (value_node.type == GdmlNodeType::BOOLEAN_EXPRESSION) {
		const BooleanNode &node = (BooleanNode &)value_node;
		obj.set(prop_key, node.value);
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
				print_error("Error constructing variant type " + node.name->name);
				return false;
			}
		} else {
			if (ClassDB::class_exists(node.name->name)) {
				resource_variant = ClassDB::instantiate(node.name->name);
			} else {
				print_error("Error instantiating class " + node.name->name);
				return false;
			}
		}

		for (const std::unique_ptr<PropertyDefinitionNode> &property : node.properties) {
			if (property->name->type == GdmlNodeType::IDENTIFIER_EXPRESSION) {
				const godot::IdentifierNode *identifier = static_cast<godot::IdentifierNode *>(property->name.get());
				setVariantProperty(resource_variant, identifier->name, *property->value.get());
			} else if (property->name->type == GdmlNodeType::MEMBER_ACCESS_EXPRESSION) {
				const godot::MemberExpressionNode *member_access = static_cast<godot::MemberExpressionNode *>(property->name.get());
				setVariantPropertyDeep(resource_variant, member_access->path, *property->value.get());
			}
		}

		obj.set(prop_key, resource_variant, &is_valid);
	}

	return is_valid;
}

bool traversal::TreeRendererVisitor::setVariantPropertyDeep(godot::Variant &obj, const std::vector<std::unique_ptr<godot::IdentifierNode>> &path, const godot::ExpressionNode &value_node) {
	godot::Variant previous_variant;
	godot::Variant current_variant = obj;

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
	if (setVariantProperty(current_variant, variant_property, value_node)) {
		const godot::String &previous_property = (path.end() - 2)->get()->name;
		previous_variant.set(previous_property, current_variant, &is_valid);
		return is_valid;
	}

	return false;
}

// Renderer

void GdmlRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("renderString"), &GdmlRenderer::renderString);
	ClassDB::bind_method(D_METHOD("renderScript"), &GdmlRenderer::renderScript);
}

GdmlRenderer::GdmlRenderer() :
		gdml(std::make_unique<GdmlParser>()) {}

Variant GdmlRenderer::renderString(const String &source) {
	std::unique_ptr<godot::SourceFileNode> result = gdml->parse(source);
	String treeString = traversal::GdmlPrinterVisitor().visit(*result.get());
	print_line(treeString);
	for (GdmlParseError &err : gdml->getParseErrors()) {
		print_line(err.toString());
	}
	return traversal::TreeRendererVisitor().visit(*result.get());
}

Variant godot::GdmlRenderer::renderScript(GdmlScript *script) {
	return renderString(script->get_source_code());
}

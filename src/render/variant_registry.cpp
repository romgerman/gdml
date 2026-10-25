#include "variant_registry.h"

using namespace godot;

namespace traversal {

std::map<godot::String, godot::Variant::Type> VariantRegistry::m_variant_type_map = {};

void VariantRegistry::initialize_map() {
	if (!m_variant_type_map.empty()) {
		return;
	}

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

bool VariantRegistry::is_builtin(const godot::String &name) {
	initialize_map();
	return m_variant_type_map.count(name) > 0;
}

bool VariantRegistry::get_constant(const godot::String &type, const godot::StringName &name, godot::Variant &out) {
	initialize_map();

	internal::gdextension_interface_variant_get_constant_value(
			(GDExtensionVariantType)m_variant_type_map[type],
			name._native_ptr(),
			&out);
	if (out.get_type() != Variant::NIL) {
		return true;
	}

	return false;
}

bool VariantRegistry::construct(const godot::String &name, godot::Variant &out) {
	initialize_map();
	
	GDExtensionCallError error;
	internal::gdextension_interface_variant_construct(
			(GDExtensionVariantType)m_variant_type_map[name],
			out._native_ptr(),
			nullptr,
			0,
			&error);

	if (error.error != GDEXTENSION_CALL_OK) {
		return false;
	}

	return true;
}

}

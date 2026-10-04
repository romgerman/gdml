#include "gdml_atom.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace reactive {

void GDMLAtom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_value"), &GDMLAtom::get_value);
	ClassDB::bind_method(D_METHOD("set_value", "_value"), &GDMLAtom::set_value);

	ADD_PROPERTY(
		PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT & ~PROPERTY_USAGE_EDITOR),
		"set_value",
		"get_value"
	);

	ADD_SIGNAL(MethodInfo("changed"));

	ClassDB::bind_method(D_METHOD("get_name"), &GDMLAtom::get_name);
}

void GDMLAtom::create(StringName p_name, Variant p_initial_value) {
	m_name = p_name;
	m_value = p_initial_value;
}

void GDMLAtom::add_dependency(Variant &p_variant, StringName p_prop) {
	Object *object = Object::cast_to<Object>(p_variant);
	ObjectID object_id = (ObjectID)object->get_instance_id();

	if (m_dependencies.find(object_id) == m_dependencies.end()) {
		m_dependencies[object_id] = std::set<StringName>();
	}

	m_dependencies[object_id].insert(p_prop);
}

bool GDMLAtom::has_dependency(Variant &p_variant, StringName p_prop) {
	Object *object = Object::cast_to<Object>(p_variant);
	ObjectID object_id = (ObjectID)object->get_instance_id();

	if (m_dependencies.find(object_id) == m_dependencies.end()) {
		return m_dependencies[object_id].count(p_prop);
	}
	
	return false;
}

void GDMLAtom::remove_dependency(Variant &p_variant, StringName p_prop) {
	Object *object = Object::cast_to<Object>(p_variant);
	ObjectID object_id = (ObjectID)object->get_instance_id();

	if (m_dependencies.find(object_id) != m_dependencies.end()) {
		m_dependencies[object_id].erase(p_prop);
	} else {
		m_dependencies.erase(object_id);
	}
}

void GDMLAtom::add_callback(std::function<void()> p_callback) {
	m_callbacks.push_back(p_callback);
}

void GDMLAtom::set_value(Variant p_value) {
	m_value = p_value;

	for (std::pair<ObjectID, std::set<StringName>> pair : m_dependencies) {
		Object *object = ObjectDB::get_instance(pair.first);
		if (object) {
			for (StringName prop : pair.second) {
				object->set(prop, p_value);
			}
		}
	}

	for (std::function<void()> fn : m_callbacks) {
		fn();
	}

	emit_signal("changed");
}

Variant GDMLAtom::get_value() {
	return m_value;
}

StringName GDMLAtom::get_name() {
	return m_name;
}

}
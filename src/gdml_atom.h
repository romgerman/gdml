#pragma once

#include <vector>
#include <map>
#include <set>

#include <godot_cpp/classes/ref_counted.hpp>

namespace reactive {

class GDMLAtom : public godot::RefCounted {
	GDCLASS(GDMLAtom, RefCounted);

private:
	godot::Variant m_value;
	godot::StringName m_name;

	std::map<godot::ObjectID, std::set<godot::StringName>> m_dependencies;

protected:
	static void _bind_methods();

public:
	void create(godot::StringName p_name, godot::Variant p_initial_value);
	void add_dependency(godot::Variant &p_variant, godot::StringName p_prop);
	void remove_dependency(godot::Variant &p_variant, godot::StringName p_prop);

	void set_value(godot::Variant p_value);
	godot::Variant get_value();

	godot::StringName get_name();
};

} //namespace reactive

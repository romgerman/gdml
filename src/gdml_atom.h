#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

namespace reactive {

class GDMLAtom : public godot::RefCounted {
	GDCLASS(GDMLAtom, RefCounted);

private:
	godot::Variant m_value;
	godot::StringName m_name;

protected:
	static void _bind_methods();

public:
	void create(godot::StringName p_name, godot::Variant p_initial_value);

	void set_value(godot::Variant p_value);
	godot::Variant get_value();

	godot::StringName get_name();
};

} //namespace reactive

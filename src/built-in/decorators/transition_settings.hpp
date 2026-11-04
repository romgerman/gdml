#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

namespace gdml {

class GDMLTransitionOptions : public godot::RefCounted {
	GDCLASS(GDMLTransitionOptions, godot::RefCounted)
protected:
	static void _bind_methods() {
		godot::ClassDB::bind_method(godot::D_METHOD("set_enabled", "enabled"), &GDMLTransitionOptions::set_enabled);
		godot::ClassDB::bind_method(godot::D_METHOD("get_enabled"), &GDMLTransitionOptions::get_enabled);

		ADD_PROPERTY(
			godot::PropertyInfo(godot::Variant::BOOL, "enabled", godot::PROPERTY_HINT_NONE),
			"set_enabled",
			"get_enabled"
		);

		godot::ClassDB::bind_method(godot::D_METHOD("set_tween_factory", "tween_factory"), &GDMLTransitionOptions::set_tween_factory);
		godot::ClassDB::bind_method(godot::D_METHOD("get_tween_factory"), &GDMLTransitionOptions::get_tween_factory);

		ADD_PROPERTY(
			godot::PropertyInfo(
				godot::Variant::CALLABLE,
				"tween_factory",
				godot::PROPERTY_HINT_NONE,
				"",
				godot::PROPERTY_USAGE_DEFAULT & ~godot::PROPERTY_USAGE_EDITOR
			),
			"set_tween_factory",
			"get_tween_factory"
		);
	}

	bool m_enabled = true;
	godot::Callable m_tween_factory;

public:
	void set_enabled(bool p_enabled) {
		m_enabled = p_enabled;
	}	
	bool get_enabled() {
		return m_enabled;
	}

	void set_tween_factory(godot::Callable p_tween_factory) {
		m_tween_factory = p_tween_factory;
	}
	godot::Callable get_tween_factory() {
		return m_tween_factory;
	}
};

class GDMLTransitionEnter : public GDMLTransitionOptions {
	GDCLASS(GDMLTransitionEnter, GDMLTransitionOptions)
protected:
	static void _bind_methods() {}
};

class GDMLTransitionExit : public GDMLTransitionOptions {
	GDCLASS(GDMLTransitionExit, GDMLTransitionOptions)
protected:
	static void _bind_methods() {}
};

}

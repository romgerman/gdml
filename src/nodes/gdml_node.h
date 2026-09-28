#pragma once

#include <godot_cpp/classes/control.hpp>

namespace godot {

class GdmlScript;
class GdmlRenderer;

class GDMLNode : public Control {
	GDCLASS(GDMLNode, Control)

private:
	Ref<GdmlScript> _markup;
	Ref<GdmlRenderer> _renderer;

protected:
	static void _bind_methods();

public:
	GDMLNode();
	~GDMLNode();

	void _ready() override;

	void set_markup(const Ref<GdmlScript> &p_markup);
	Ref<GdmlScript> get_markup();
};

}

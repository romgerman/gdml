#pragma once

#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_traversal.h"

#include <map>
#include <memory>

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

class GdmlRenderer : public RefCounted {
	GDCLASS(GdmlRenderer, RefCounted)

	std::unique_ptr<GdmlParser> m_parser;

protected:
	static void _bind_methods();

public:
	GdmlRenderer();
	Variant renderString(const String &source, const godot::Dictionary &data);
	Variant renderScript(GdmlScript *script, const godot::Dictionary &data);
};

} //namespace godot
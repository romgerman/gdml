#pragma once

#include <godot_cpp/classes/resource_format_saver.hpp>

namespace godot {

class GdmlScriptResourceFormatSaver : public ResourceFormatSaver {
    GDCLASS(GdmlScriptResourceFormatSaver, ResourceFormatSaver);

protected:
	static void _bind_methods();

public:
	Error _save(const Ref<Resource> &resource, const String &path, uint32_t flags) override;
	bool _recognize(const Ref<Resource> &resource) const override;
	PackedStringArray _get_recognized_extensions(const Ref<Resource> &resource) const override;

	static void register_in_godot();
	static void unregister_in_godot();
};

}
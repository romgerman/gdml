#include "gdml_script_resource_format_saver.h"
#include "gdml_script.h"
#include "gdml_script_language.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

using namespace godot;

static Ref<GdmlScriptResourceFormatSaver> instance;

void GdmlScriptResourceFormatSaver::_bind_methods() {}

Error GdmlScriptResourceFormatSaver::_save(const Ref<Resource> &resource,
                                                  const String &p_path,
                                                  uint32_t flags) {
  Ref<GdmlScript> script = resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::WRITE);
	ERR_FAIL_COND_V_MSG(file.is_null(), FileAccess::get_open_error(), "Failed to save file at " + p_path);

	file->store_string(script->get_source_code());
	Error error = file->get_error();
	switch (error) {
		case OK:
		case ERR_FILE_EOF:
			return OK;
		default:
			return error;
	}
}

bool GdmlScriptResourceFormatSaver::_recognize(const Ref<Resource> &resource) const {
    Ref<GdmlScript> script = resource;
    return script.is_valid();
}

PackedStringArray GdmlScriptResourceFormatSaver::_get_recognized_extensions(const Ref<Resource> &resource) const {
    PackedStringArray extensions;
	if (_recognize(resource)) {
		extensions.push_back("gdml");
	}
	return extensions;
}

void GdmlScriptResourceFormatSaver::register_in_godot() {
    instance.instantiate();
	ResourceSaver::get_singleton()->add_resource_format_saver(instance);
}

void GdmlScriptResourceFormatSaver::unregister_in_godot() {
    ResourceSaver::get_singleton()->remove_resource_format_saver(instance);
	instance.unref();
}

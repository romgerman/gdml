#include "register_types.h"

#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_syntax_highlighter.h"
#include "gdml_script_resource_format_loader.h"
#include "gdml_script_resource_format_saver.h"
#include "gdml_renderer.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_example_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	
	GDREGISTER_CLASS(GdmlScript);
	GDREGISTER_CLASS(GdmlScriptLanguage)
	GDREGISTER_CLASS(GdmlScriptResourceFormatLoader)
	GDREGISTER_CLASS(GdmlScriptResourceFormatSaver)
	GDREGISTER_CLASS(GdmlSyntaxHighlighter);
	GDREGISTER_CLASS(GdmlRenderer);

	GdmlScriptLanguage::get_or_create_singleton();
	GdmlScriptResourceFormatLoader::register_in_godot();
	GdmlScriptResourceFormatSaver::register_in_godot();
	print_line("initialized");

	GdmlRenderer* render = memnew(GdmlRenderer);
	render->renderString("Control {  test_prop: \"test value\"; }");
}

void uninitialize_example_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GdmlScriptResourceFormatSaver::unregister_in_godot();
	GdmlScriptResourceFormatLoader::unregister_in_godot();
	GdmlScriptLanguage::delete_singleton();
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT example_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_example_module);
	init_obj.register_terminator(uninitialize_example_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
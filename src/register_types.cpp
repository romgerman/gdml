#include "register_types.h"

#include "gdml_editor_plugin.h"
#include "gdml_script.h"
#include "gdml_script_language.h"
#include "gdml_script_resource_format_loader.h"
#include "gdml_script_resource_format_saver.h"
#include "gdml_syntax_highlighter.h"

#include "gdml_atom.h"
#include "gdml_global.h"
#include "gdml_decorator.h"
#include "gdml_function.h"

#include "built-in/functions/load.hpp"
#include "built-in/decorators/transition_settings.hpp"
#include "built-in/decorators/transition.hpp"

#include "render/gdml_renderer.h"

#include "nodes/gdml_node.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_gdml_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_SCENE: {
			GDREGISTER_CLASS(GdmlScript);
			GDREGISTER_CLASS(GdmlScriptLanguage);
			GDREGISTER_CLASS(GdmlScriptResourceFormatLoader);
			GDREGISTER_CLASS(GdmlScriptResourceFormatSaver);
			GDREGISTER_CLASS(GdmlSyntaxHighlighter);

			GDREGISTER_CLASS(reactive::GDMLAtom);
			GDREGISTER_CLASS(gdml::GDMLResult);
			GDREGISTER_CLASS(gdml::GDMLRenderer);
			GDREGISTER_CLASS(global::GDML);

			GDREGISTER_CLASS(GDMLNode);
			GDREGISTER_CLASS(gdml::GDMLDecorator);
			GDREGISTER_CLASS(gdml::GDMLFunction);

			// Register built-ins
			GDREGISTER_CLASS(gdml::GDMLFunction_load);
			GDREGISTER_CLASS(gdml::GDMLTransitionOptions);
			GDREGISTER_CLASS(gdml::GDMLTransitionEnter);
			GDREGISTER_CLASS(gdml::GDMLTransitionExit);
			GDREGISTER_CLASS(gdml::GDMLDecorator_transition);

			GdmlScriptLanguage::get_or_create_singleton();
			GdmlScriptResourceFormatLoader::register_in_godot();
			GdmlScriptResourceFormatSaver::register_in_godot();
		} break;

		case MODULE_INITIALIZATION_LEVEL_EDITOR: {
			GDREGISTER_CLASS(GdmlEditorPlugin);

			EditorPlugins::add_by_type<GdmlEditorPlugin>();
		} break;
	}

	print_line("Gdml initialized");
}

void uninitialize_gdml_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_SCENE: {
			GdmlScriptResourceFormatSaver::unregister_in_godot();
			GdmlScriptResourceFormatLoader::unregister_in_godot();
			GdmlScriptLanguage::delete_singleton();
		} break;

		case MODULE_INITIALIZATION_LEVEL_EDITOR: {
			EditorPlugins::remove_by_type<GdmlEditorPlugin>();
		} break;
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT gdml_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_gdml_module);
	init_obj.register_terminator(uninitialize_gdml_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
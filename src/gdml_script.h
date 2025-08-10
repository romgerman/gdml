#pragma once

#include <godot_cpp/classes/script_extension.hpp>

namespace godot {

class GdmlScript : public ScriptExtension {
    GDCLASS(GdmlScript, ScriptExtension)

protected:
    static void _bind_methods();

    String source_code;

public:
    ScriptLanguage *_get_language() const override;
    
    bool _has_static_method(const StringName &p_method) const override;
    Ref<Script> _get_base_script() const override;
	StringName _get_global_name() const override;
    StringName _get_instance_base_type() const override;
    
    virtual String _get_source_code() const override;
    void _set_source_code(const String &p_code) override;
    bool _has_source_code() const override;
    
    Error _reload(bool p_keep_state) override;
    bool _editor_can_reload_from_file() override;
    void _update_exports() override;
    bool _is_tool() const override;
    bool _is_abstract() const override;
    void _placeholder_erased(void *p_placeholder) override;
    bool _can_instantiate() const override;
    TypedArray<Dictionary> _get_documentation() const override;

public:
    //void render();
};

}
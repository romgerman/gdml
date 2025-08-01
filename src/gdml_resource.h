#pragma once

#include <godot_cpp/classes/resource.hpp>

namespace godot {

class GdmlResource : public Resource {
    GDCLASS(GdmlResource, Resource)

protected:
    static void _bind_methods();

public:
    GdmlResource();
    ~GdmlResource();
};

}
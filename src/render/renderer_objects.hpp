#pragma once

#include <map>
#include <set>

#include <godot_cpp/variant/variant.hpp>

namespace gdml {

struct SignalTargetItem {
	std::set<godot::StringName> signals;
	godot::Array args;
};

typedef std::map<godot::ObjectID, SignalTargetItem> SignalTarget;

}

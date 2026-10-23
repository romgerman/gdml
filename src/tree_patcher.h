#pragma once

#include "diff.h"

#include <vector>

#include <godot_cpp/classes/node.hpp>

namespace gdml {

class TreePatcher {
public:
	void patch(godot::Node *node, const std::vector<DiffItem> &diff);
};

}

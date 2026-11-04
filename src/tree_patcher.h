#pragma once

#include "diff.h"

#include <vector>

namespace gdml {

class TreePatcher {
public:
	void patch(const std::vector<DiffItem> &diff);
};

} //namespace gdml

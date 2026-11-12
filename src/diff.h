#pragma once

#include <vector>
#include <map>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace gdml {

enum class DiffOp {
	INSERT,
	DELETE,
	UPDATE,
	MOVE
};

struct DiffItem {
	DiffOp type;
	godot::Node *old_node = nullptr;
	godot::Node *node = nullptr;
	godot::Node *parent = nullptr;
	int32_t old_index;
	int32_t index;
	std::vector<godot::StringName> props;
};

class Differ {
public:
	void diff(
		std::vector<godot::Node*> &nodes_a,
		std::vector<godot::Node*> &nodes_b,
		godot::Node *parent_a,
		std::vector<DiffItem> &out_diff,
		std::map<godot::ObjectID, godot::ObjectID> &remaps
	);
	bool is_node_equal(godot::Node *a, godot::Node *b);
	void diff_node_props(godot::Node *a, godot::Node *b, std::vector<godot::StringName> &out_props);
};

} // namespace gdml

#include "gdml_syntax_highlighter.h"

#include "gdml_parser.h"
#include "tree_sitter.h"

#include <functional>

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/text_edit.hpp>

using namespace godot;

void GdmlSyntaxHighlighter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_syntax_highlighting", "text", "line"), &GdmlSyntaxHighlighter::get_syntax_highlighting);
}

Dictionary godot::GdmlSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
	const godot::String text = get_text_edit()->get_line(p_line);
	return Dictionary();
}

Dictionary godot::GdmlSyntaxHighlighter::get_syntax_highlighting(godot::String p_text, int32_t p_line) const {
	godot::Dictionary out;

	auto tree = GdmlParser().rawParse(p_text);
	ts::Node root_node = ts::Node::getRootNode(tree.get());
	std::vector<std::tuple<ts::Node, godot::String>> node_on_line = find_nodes_on_line(root_node, p_line);

	for (std::tuple<ts::Node, godot::String> &node : node_on_line) {
		godot::Color color = get_color_for_node(node);
		godot::Dictionary line_out;
		if (color.a != 0.0f) {
			line_out.set("color", color);
		}
		out.set(Variant(std::get<ts::Node>(node).getStartPoint().column), line_out);
	}

	return out;
}

std::vector<std::tuple<ts::Node, godot::String>> godot::GdmlSyntaxHighlighter::find_nodes_on_line(const ts::Node &root_node, int32_t line) const {
	std::vector<std::tuple<ts::Node, godot::String>> out;

	static const std::set<godot::String> possible_parent = {
		"node_definition",
		//"property_definition",
		"resource_definition",
		"function_call"
	};

	godot::String current_parent;

	std::function<void(const ts::Node)> traverse = [&](const ts::Node node) {
		if (node.getStartPoint().row == line) {
			out.push_back(std::tuple(node, current_parent));
		}

		if (possible_parent.find(node.getType()) != possible_parent.end()) {
			current_parent = node.getType();
		} else {
			current_parent = String();
		}

		for (uint32_t i = 0; i < node.getChildCount(); i++) {
			const ts::Node child = node.getChild(i);
			if (child.getType() != "ERROR") {
				traverse(child);
			}
		}
	};

	traverse(root_node);

	return out;
}

godot::Color godot::GdmlSyntaxHighlighter::get_color_for_node(std::tuple<ts::Node, godot::String> &node) const {
	const godot::String type = std::get<ts::Node>(node).getType();

	static const std::set<godot::String> possible_parent = {
		"node_definition",
		//"property_definition",
		"resource_definition",
		"function_call"
	};

	auto editor_settings = EditorInterface::get_singleton()->get_editor_settings();
	Color comment_color = editor_settings->get_setting("text_editor/theme/highlighting/comment_color");
	Color keyword_color = editor_settings->get_setting("text_editor/theme/highlighting/keyword_color");
	Color engine_type_color = editor_settings->get_setting("text_editor/theme/highlighting/engine_type_color");
	Color member_color = editor_settings->get_setting("text_editor/theme/highlighting/member_variable_color");
	Color number_color = editor_settings->get_setting("text_editor/theme/highlighting/number_color");
	Color string_color = editor_settings->get_setting("text_editor/theme/highlighting/string_color");
	Color symbol_color = editor_settings->get_setting("text_editor/theme/highlighting/symbol_color");
	Color text_color = editor_settings->get_setting("text_editor/theme/highlighting/text_color");
	//Color function_color = editor_settings->get_setting("text_editor/theme/highlighting/function_color");
	//Color user_type_color = editor_settings->get_setting("text_editor/theme/highlighting/user_type_color");

	if (type == "comment") {
		return comment_color;
	} else if (type == "signal_expression" || type == "signal_definition" || type == "variable_definition") {
		return keyword_color;
	} else if (type == "identifier") {
		if (possible_parent.find(std::get<godot::String>(node)) != possible_parent.end()) {
			return engine_type_color;
		}
		return member_color;
	} else if (type == "number") {
		return number_color;
	} else if (type == "string" || type == "string_fragment" || type == "escape_sequence") {
		return string_color;
	} else if (type == "boolean" || type == "true" || type == "false") {
		return keyword_color;
	} else if (type == "{" || type == "}" || type == "[" || type == "]") {
		return symbol_color;
	} else {
		return godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

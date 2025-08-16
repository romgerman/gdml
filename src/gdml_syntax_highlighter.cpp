#include "gdml_syntax_highlighter.h"

#include "gdml_parser.h"
#include "tree_sitter.h"

#include <tree_sitter/tree-sitter-gdml.h>

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
	std::vector<ts::Node> node_on_line = find_nodes_on_line(root_node, p_line);

	for (ts::Node &node : node_on_line) {
		godot::Color color = get_color_for_node(node);
		godot::Dictionary line_out;
		line_out.set("color", color);
		out.set(Variant(node.getStartPoint().column), line_out);
	}

	return out;
}

std::vector<ts::Node> godot::GdmlSyntaxHighlighter::find_nodes_on_line(ts::Node root_node, int32_t line) const {
	std::vector<ts::Node> out;

	std::function<void(ts::Node)> traverse = [&](ts::Node node) {
		if (node.getStartPoint().row == line) {
			out.push_back(node);
		}

		for (uint32_t i = 0; i < node.getChildCount(); i++) {
			ts::Node child = node.getChild(i);
			traverse(child);
		}
	};

	traverse(root_node);

	return out;
}

godot::Color godot::GdmlSyntaxHighlighter::get_color_for_node(ts::Node &node) const {
	const godot::String type = node.getType();

	auto editor_settings = EditorInterface::get_singleton()->get_editor_settings();
	Color comment_color = editor_settings->get_setting("text_editor/theme/highlighting/comment_color");
	Color keyword_color = editor_settings->get_setting("text_editor/theme/highlighting/keyword_color");
	Color member_color = editor_settings->get_setting("text_editor/theme/highlighting/member_variable_color");
	Color number_color = editor_settings->get_setting("text_editor/theme/highlighting/number_color");
	Color string_color = editor_settings->get_setting("text_editor/theme/highlighting/string_color");
	Color symbol_color = editor_settings->get_setting("text_editor/theme/highlighting/symbol_color");
	Color text_color = editor_settings->get_setting("text_editor/theme/highlighting/text_color");
	//Color user_type_color = editor_settings->get_setting("text_editor/theme/highlighting/user_type_color");

	if (type == "comment") {
		return comment_color;
	} else if (type == "identifier") {
		return member_color;
	} else if (type == "number") {
		return number_color;
	} else if (type == "string") {
		return string_color;
	} else if (type == "boolean") {
		return keyword_color;
	} else if (type == "{" || type == "}" || type == "[" || type == "]") {
		return symbol_color;
	} else {
		return text_color;
	}
}

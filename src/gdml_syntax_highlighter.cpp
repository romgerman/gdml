#include "gdml_syntax_highlighter.h"

#include "tree_sitter.h"

#include <functional>

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/text_edit.hpp>

using namespace godot;

namespace godot {

void GdmlSyntaxHighlighter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_syntax_highlighting", "text", "line"), &GdmlSyntaxHighlighter::get_syntax_highlighting);
}

Dictionary GdmlSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
	const String text = get_text_edit()->get_line(p_line);
	return get_syntax_highlighting(text, p_line);
}

Dictionary GdmlSyntaxHighlighter::get_syntax_highlighting(const String &p_text, int32_t p_line) const {
	Dictionary out;

	CharString source = p_text.utf8();
	auto tree = _parser.rawParse(source);
	ts::Node root_node = ts::Node::getRootNode(tree.get());
	std::vector<std::tuple<ts::Node, String, bool>> node_on_line = find_nodes_on_line(root_node, p_line);

	//print_line(ts_node_string(root_node.raw()));

	auto editor_settings = EditorInterface::get_singleton()->get_editor_settings();
	Color error_color = editor_settings->get_setting("text_editor/theme/highlighting/error_underline_color");

	for (std::tuple<ts::Node, String, bool> &pair : node_on_line) {
		const ts::Node &node = std::get<ts::Node>(pair);
		Variant start_key = Variant(node.getStartPoint().column);
		Variant end_key = Variant(node.getEndPoint().column);
		Color color = get_color_for_node(pair);
		Dictionary line_out;
		if (color.a != 0.0f) {
			line_out.set("color", color);
			out.set(start_key, line_out);
			if (node.getEndPoint().row == p_line && !out.has(end_key)) {
				out.set(end_key, Dictionary());
			}
		} else if (!out.has(start_key)) {
			out.set(start_key, line_out);
		}
	}

	return out;
}

std::vector<std::tuple<ts::Node, String, bool>> GdmlSyntaxHighlighter::find_nodes_on_line(const ts::Node &root_node, int32_t line) const {
	std::vector<std::tuple<ts::Node, String, bool>> out;

	static const std::set<String> possible_parent = {
		"node_definition",
		//"property_definition",
		"resource_definition",
		"function_call"
	};

	std::function<void(const ts::Node, const String, bool)> traverse = [&](const ts::Node &node, const String &parent, bool isError) {
		bool error = isError || node.isError();

		String next_parent = possible_parent.count(node.getType()) ? node.getType() : String();

		for (uint32_t i = 0; i < node.getChildCount(); i++) {
			const ts::Node child = node.getChild(i);
			if (!child.isError()) {
				traverse(child, next_parent, error);
			}
		}

		if (node.getStartPoint().row == line) {
			if (node.isType("string_fragment") || node.isType("escape_sequence")) {
				return;
			}

			out.push_back(std::tuple(node, parent, error));
		}
	};

	traverse(root_node, String(), root_node.isError());

	return out;
}

Color GdmlSyntaxHighlighter::get_color_for_node(std::tuple<ts::Node, String, bool> &pair) const {
	const ts::Node &node = std::get<ts::Node>(pair);
	const bool isError = std::get<bool>(pair);
	const String &parent = std::get<String>(pair);
	const String type = node.getType();

	static const std::set<String> possible_parent = {
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
	Color error_color = editor_settings->get_setting("text_editor/theme/highlighting/error_underline_color");
	Color control_flow_color = editor_settings->get_setting("text_editor/theme/highlighting/control_flow_keyword_color");
	//Color base_type_color = editor_settings->get_setting("text_editor/theme/highlighting/base_type_color");

	if (isError) {
		return error_color;
	} else if (type == "comment") {
		return comment_color;
	} else if (type == "for_statement" || type == "if_statement") {
		return control_flow_color;
	} else if (type == "signal_definition" || type == "variable_definition" || type == "import_statement" || type == "component_definition") {
		return keyword_color;
	} else if (type == "signal_expression" || type == "theme_property_definition") {
		return keyword_color;
	} else if (type == "identifier") {
		if (possible_parent.find(parent) != possible_parent.end()) {
			return engine_type_color;
		}
		return member_color;
	} else if (type == "number") {
		return number_color;
	} else if (type == "string") {
		return string_color;
	} else if (type == "boolean" || type == "true" || type == "false" || type == "self" || type == "from") {
		return keyword_color;
	} else if (type == "{" || type == "}" || type == "[" || type == "]") {
		return symbol_color;
	} else {
		return Color(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

}

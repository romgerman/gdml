@tool
extends EditorSyntaxHighlighter

var real: GdmlSyntaxHighlighter

func _init() -> void:
	real = GdmlSyntaxHighlighter.new()

func _get_name() -> String:
	return "GDML"

func _get_supported_languages() -> PackedStringArray:
	return ["gdml"]

func _get_line_syntax_highlighting(line: int) -> Dictionary:
	return real.get_syntax_highlighting(get_text_edit().text, line)

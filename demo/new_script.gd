@tool
extends Control

@export_tool_button("render")
var _render = render
@export var what: GdmlScript

func render() -> void:
	var renderer := GDMLRenderer.new()
	var scriptResult := renderer.renderMarkup(what)
	#var item_count := scriptResult.get_variable("item_count")
	var item_color := scriptResult.get_variable("item_color")
	item_color.value = Color.BLACK
	
	print(scriptResult.get_signals())

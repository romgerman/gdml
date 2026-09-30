@tool
extends Control

@export_tool_button("render")
var _render = render
@export var what: GdmlScript

enum {
	GROW_POOP = 0
}

enum Named {
	GROW_POOP = 345
}

func render() -> void:
	var renderer = GDMLRenderer.new()
	var scriptResult = renderer.renderMarkup(what)
	print("rendered script ", scriptResult.get_variable("item_count").value)

#func _ready() -> void:
#	var renderer = GdmlRenderer.new()
#	var scriptResult = renderer.renderScript(what, {
#		"timeString": "123123123"
#	})
#	print("rendered script ", scriptResult)
#	add_child(scriptResult)
#	await get_tree().process_frame
#	print(scriptResult.size.x)

extends Control

@export var what: GdmlScript

func _ready() -> void:
	var renderer = GdmlRenderer.new()
	var scriptResult = renderer.renderScript(what, {
		"timeString": "123123123"
	})
	print("rendered script ", scriptResult)
	add_child(scriptResult)
	await get_tree().process_frame
	print(scriptResult.size.x)

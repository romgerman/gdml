extends CanvasLayer

@export var ui_script: GdmlScript

func _ready() -> void:
	var result = GdmlRenderer.new().renderScript(ui_script, {
		"game_name": "My Super Cool Game!"
	})
	add_child(result)

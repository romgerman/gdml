extends CanvasLayer

@export var ui_script: GdmlScript

func _ready() -> void:
	var result = GdmlRenderer.new().renderScript(ui_script, {
		"game_name": "My Super Cool Game!",
		"continue_pressed": Callable(self, "_continue_pressed")
	})
	add_child(result)

func _continue_pressed():
	print("continue pressed")

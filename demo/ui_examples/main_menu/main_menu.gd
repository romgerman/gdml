extends CanvasLayer

@export var ui_script: GdmlScript

func _ready() -> void:
	var result = GdmlRenderer.new().renderMarkup(ui_script, {
		"game_name": "My Super Cool Game!",
		"continue_pressed": _continue_pressed,
		"load": _load_res,
		"test_raw_fn": _raw_fn
	})
	for n in result.get_nodes():
		add_child(n)

func _raw_fn():
	return "text from fn"

func _continue_pressed():
	print("continue pressed")

func _load_res(path: String):
	print("yay it called")
	return load(path)

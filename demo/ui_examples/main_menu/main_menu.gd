extends CanvasLayer

@export var ui_script: GdmlScript

func _ready() -> void:
	var result = GDMLRenderer.new().renderMarkup(self, ui_script)

func _raw_fn():
	return "text from fn"

func _continue_pressed():
	print("continue pressed")

func _load_res(path: String):
	print("yay it called")
	return load(path)

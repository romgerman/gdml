extends Node

@export var what: GdmlScript

func _ready() -> void:
	var renderer = GdmlRenderer.new()
	var stringResult = renderer.renderString("Control {}")
	print("rendered string ", stringResult)
	var scriptResult = renderer.renderScript(what)
	print("rendered script ", scriptResult)

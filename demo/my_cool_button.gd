@tool
extends Button
class_name MyCoolButton

var time: float
var timeString: String
var timeStringAtom: GDMLAtom = GDML.atom(&"time")

func _ready() -> void:
	print("My Cool Button is ready")

func _process(delta: float) -> void:
	time = Time.get_ticks_msec()
	timeString = str(time) + "ms"

static func load_res(path: String) -> Resource:
	print("loading: ", path)
	return load(path)

@tool
extends GDMLDecorator
class_name GDMLDecorator_anchors

func modify(input: Variant, args: Array, _context: GDMLResult):
	if args.is_empty():
		return
	var control: Control = input
	control.anchor_left = args[0] if args[0] else 0.0
	control.anchor_top = args[1] if args[1] else 0.0
	control.anchor_right = args[2] if args[2] else 0.0
	control.anchor_bottom = args[3] if args[3] else 0.0

func get_decorator_name() -> Variant:
	return &"anchors"

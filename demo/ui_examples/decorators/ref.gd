@tool
extends GDMLDecorator
class_name GDMLDecorator_ref

func modify(control: Variant, args: Array, _context: GDMLResult) -> void:
	if args.size() > 0:
		if args[0] is Dictionary and args.size() > 1:
			var map := args[0] as Dictionary
			var key = args[1]
			map[key] = control

func get_decorator_name() -> Variant:
	return &"ref"

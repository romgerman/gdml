@tool
extends GDMLDecorator
class_name GDMLDecorator_ref

func modify(control: Variant, args: Array, _context: GDMLResult) -> void:
	if args.size() > 0:
		if args.size() > 1 and args[0] is Dictionary:
			var map := args[0] as Dictionary
			var key = args[1]
			map[key] = control
		elif args[0] is Array:
			# TODO: ...
			var arr := args[0] as Array
			arr.push_back(control)

func get_decorator_name() -> Variant:
	return &"ref"

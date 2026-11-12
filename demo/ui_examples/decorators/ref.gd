@tool
extends GDMLDecorator
class_name GDMLDecorator_ref

var refs_dict := {}

func before_render() -> void:
	for arr: Array in refs_dict.values():
		arr.clear()

func modify(control: Variant, args: Array, context: GDMLResult) -> void:
	if args.size() > 0:
		if args.size() > 1 and args[0] is Dictionary:
			var map := args[0] as Dictionary
			var key = args[1]
			map[key] = control
		elif args[0] is Array:
			var arr := refs_dict.get_or_add(context, []) as Array
			arr.push_back(control)

func get_decorator_name() -> Variant:
	return &"ref"

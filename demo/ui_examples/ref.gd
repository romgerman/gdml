@tool
extends GDMLDecorator
class_name GDMLDecorator_ref

static var ref_list := {}

func modify(control: Variant, args: Array) -> void:
	if args.size() > 0:
		if args[0] is Dictionary and args.size() > 1:
			var map := args[0] as Dictionary
			var key = args[1]
			map[key] = control
		else:
			ref_list[args[0]] = control

static func get_ref(name: String) -> Control:
	return ref_list.get(name)

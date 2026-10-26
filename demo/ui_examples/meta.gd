@tool
extends GDMLDecorator
class_name GDMLDecorator_meta

func modify(control: Variant, args: Array) -> void:
	if args.size() < 2:
		return
	
	var key: String = args[0]
	var value = args[1]
	(control as Node).set_meta(key, value)

func get_decorator_name() -> Variant:
	return &"meta"

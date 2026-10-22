@tool
extends GDMLDecorator
class_name GDMLDecorator_border

func modify_resource(res: Variant, args: Array) -> Variant:
	if res is StyleBoxFlat:
		res.border_width_left = args[0] if args[0] else 0
		res.border_width_top = args[1] if args[1] else 0
		res.border_width_right = args[2] if args[2] else 0
		res.border_width_bottom = args[3] if args[3] else 0
		
		if args.size() > 4 and args[4] is Color:
			res.border_color = args[4]
	return res

func get_decorator_name() -> Variant:
	return &"border"

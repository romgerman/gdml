@tool
extends GDMLDecorator
class_name GDMLDecorator_expand_margin

func modify_resource(res: Variant, args: Array) -> Variant:
	if res is StyleBoxFlat:
		res.expand_margin_left = args[0] if args[0] else 0
		res.expand_margin_top = args[1] if args[1] else 0
		res.expand_margin_right = args[2] if args[2] else 0
		res.expand_margin_bottom = args[3] if args[3] else 0
	return res

@tool
extends GDMLNode

class ItemStack:
	var item: InventoryItem
	var amount: int = 1

@export var inventory_size: int = 15
@export var columns: int = 5
@export var items: Array[InventoryItem] = []

var inventory := []

func _ready() -> void:
	init_inventory()
	
	if markup:
		markup.changed.connect(func ():
			get_variable("inventory").value = inventory
		)
	
	get_variable("columns").value = columns
	get_variable("inventory").value = inventory
	
	(func ():
		var tw := create_tween().set_trans(Tween.TRANS_BOUNCE)
		var item_refs: Dictionary = get_variable("item_refs").value
		for i in inventory_size:
			var n: Control = item_refs.get(i)
			n.offset_transform_scale = Vector2.ZERO
			tw.parallel().tween_property(n, "offset_transform_scale", Vector2.ONE, 0.45)
	).call_deferred()
	
	if not Engine.is_editor_hint():
		connect("item_gui_input", _item_gui_input)

func init_inventory() -> void:
	for i in inventory_size:
		var stack := ItemStack.new()
		stack.amount = randi_range(1, 10)
		inventory.push_back(stack)
	
	for i in items.size():
		inventory[i].item = items[i]

var drag_ctrl: Control
var active_drop_target: DropTarget
var item_index: int

func _item_gui_input(target: Control, event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.is_pressed():
			if not drag_ctrl:
				drag_ctrl = target
				drag_ctrl.z_index = 999
				var dt := drag_ctrl.get_parent() as DropTarget
				item_index = dt.index
				
				var tw := create_tween().set_trans(Tween.TRANS_QUAD)
				tw.tween_property(drag_ctrl, "offset_transform_scale", Vector2.ONE * 0.9, 0.35)
		elif event.button_index == MOUSE_BUTTON_LEFT and event.is_released():
				if drag_ctrl:
					if active_drop_target:
						var target_index := active_drop_target.index
						var a = inventory[item_index]
						inventory[item_index] = inventory[target_index]
						inventory[target_index] = a
						
						get_variable("inventory").value = inventory
						
						(func ():
							var item_refs: Dictionary = get_variable("item_refs").value
							var my_target: Control = item_refs.get(target_index)
							my_target.modulate = Color.TRANSPARENT
							var tw := create_tween().set_trans(Tween.TRANS_QUAD)
							tw.parallel().tween_property(my_target, "modulate", Color.WHITE, 1.0)
							var item_ctrl := my_target.get_node(^"DropTarget/Item") as Control

							item_ctrl.offset_transform_scale = Vector2.ONE * 0.9
							tw.parallel().tween_property(item_ctrl, "offset_transform_scale", Vector2.ONE, 0.35)
						).call_deferred()
						
						active_drop_target.remove_theme_stylebox_override(&"panel")
						active_drop_target = null
						
					drag_ctrl.z_index = 0
					drag_ctrl.position = Vector2.ZERO
					drag_ctrl = null

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		if drag_ctrl:
			drag_ctrl.position += event.relative
			
			var targets := find_children("*", "DropTarget", true, false)
			
			for t: DropTarget in targets:
				if t.get_global_rect().has_point(event.global_position):
					active_drop_target = t
					#t.add_theme_stylebox_override(&"panel", get_variable("dt_stylebox").value)
					_animate_stylbox_flat(t, &"panel", get_variable("dt_stylebox").value)
				else:
					if active_drop_target:
						t.remove_theme_stylebox_override(&"panel")

var h_ctrl: Control

func _animate_stylbox_flat(ctrl: Control, sb_name: StringName, new_stylebox: StyleBoxFlat) -> void:
	if h_ctrl != ctrl:
		var sb := ctrl.get_theme_stylebox(sb_name).duplicate() as StyleBoxFlat
		sb.border_color = Color.TRANSPARENT
		sb.border_width_bottom = new_stylebox.border_width_bottom
		sb.border_width_top = new_stylebox.border_width_top
		sb.border_width_left = new_stylebox.border_width_left
		sb.border_width_right = new_stylebox.border_width_right
		sb.expand_margin_bottom = new_stylebox.expand_margin_bottom
		sb.expand_margin_top = new_stylebox.expand_margin_top
		sb.expand_margin_left = new_stylebox.expand_margin_left
		sb.expand_margin_right = new_stylebox.expand_margin_right
		ctrl.add_theme_stylebox_override(sb_name, sb)
		var tw := create_tween()
		tw.tween_property(sb, "border_color", new_stylebox.border_color, 0.25)
		tw.play()
		h_ctrl = ctrl

@tool
extends GDMLNode

@export var inventory_size: int = 15
@export var columns: int = 5
@export var items: Array[InventoryItem] = []

func _ready() -> void:
	if markup:
		markup.changed.connect(func ():
			get_variable("items").value = items
		)
	
	get_variable("columns").value = columns
	get_variable("inventory_size").value = inventory_size
	get_variable("items").value = items
	
	if not Engine.is_editor_hint():
		connect("item_gui_input", _item_gui_input)
		pass
	
	#await get_tree().create_timer(2.0).timeout
	#var item_count := get_variable("item_count")
	#item_count.value = 3

var drag_ctrl: Control
var active_drop_target: DropTarget
var item_index: int

func _item_gui_input(target: Control, event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			if event.is_pressed():
				if not drag_ctrl:
					drag_ctrl = target
					drag_ctrl.z_index = 999
					item_index = (drag_ctrl.get_parent() as DropTarget).index
			elif event.is_released():
				if drag_ctrl:
					if active_drop_target:
						var target_index := active_drop_target.index
						var a = items[item_index]
						items[item_index] = items[target_index]
						items[target_index] = a
						get_variable("items").value = items
						
						active_drop_target.remove_theme_stylebox_override(&"panel")
						active_drop_target = null
					
					drag_ctrl.z_index = 0
					drag_ctrl.position = Vector2.ZERO
					drag_ctrl = null
	if event is InputEventMouseMotion:
		if drag_ctrl:
			drag_ctrl.position += event.relative
			
			var targets := find_children("*", "DropTarget", true, false)
			
			for t: DropTarget in targets:
				if drag_ctrl.get_parent() == t:
					continue
				if t.get_global_rect().has_point(event.global_position):
					active_drop_target = t
					t.add_theme_stylebox_override(&"panel", get_variable("dt_stylebox").value)
					break
				else:
					active_drop_target = null
					t.remove_theme_stylebox_override(&"panel")
			

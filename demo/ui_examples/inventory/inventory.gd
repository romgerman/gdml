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
	
	if not Engine.is_editor_hint():
		connect("item_gui_input", _item_gui_input)
		pass
	
func init_inventory() -> void:
	for i in inventory_size:
		var stack := ItemStack.new()
		if randf() > 0.5:
			stack.item = items[randi_range(0, items.size() - 1)]
		stack.amount = randi_range(1, 10)
		inventory.push_back(stack)
		print(stack)

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
						var a = inventory[item_index]
						inventory[item_index] = inventory[target_index]
						inventory[target_index] = a
						get_variable("inventory").value = inventory
						
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
			

extends GDMLNode

var is_dragging := false
var dragged_ctrl: Control
var dragged_index := -1

var items: GDMLAtom

func _ready() -> void:
	items = get_variable("items")
	connect("item_drag_handle_input", _on_item_drag_handle_input)
	connect("add_items", func (_c):
		var vals := items.value as Array
		for i in 5:
			vals.push_back(vals.back() + 1)
		items.value = vals
	)

func _process(delta: float) -> void:
	var ctrls := get_variable_value("item_refs") as Dictionary
	
	if is_dragging:
		for c: Control in ctrls.values():
			if c == dragged_ctrl:
				continue
			var offset_start_y := dragged_ctrl.global_position.y + dragged_ctrl.offset_transform_position.y
			var offset_end_y := dragged_ctrl.get_global_rect().end.y + dragged_ctrl.offset_transform_position.y
			if offset_start_y < c.global_position.y:
				c.offset_transform_position.y = lerpf(
					c.offset_transform_position.y,
					dragged_ctrl.get_global_rect().end.y - dragged_ctrl.global_position.y,
					delta * 10.0
				)
			elif offset_end_y > c.global_position.y:
				c.offset_transform_position.y = lerpf(
					c.offset_transform_position.y,
					dragged_ctrl.global_position.y - dragged_ctrl.get_global_rect().end.y,
					delta * 10.0
				)
	else:
		for c: Control in ctrls.values():
			c.offset_transform_position.y = lerpf(
				c.offset_transform_position.y,
				0.0,
				delta * 10.0
			)

func _gui_input(event: InputEvent) -> void:
	if is_dragging and dragged_ctrl:
		if event is InputEventMouseMotion:
			dragged_ctrl.offset_transform_position.y += event.relative.y
		elif event is InputEventMouseButton:
			if event.button_index == MOUSE_BUTTON_LEFT and event.is_released():
				_drag_end()

func _on_item_drag_handle_input(ctrl: Control, e: InputEvent) -> void:
	if not is_dragging:
		if e is InputEventMouseButton:
			if e.button_index == MOUSE_BUTTON_LEFT and e.pressed:
				_drag_start(ctrl)

func _drag_end() -> void:
	dragged_ctrl.z_index = 0
	var ctrls := get_variable_value("item_refs") as Dictionary
	var offset_y := dragged_ctrl.get_global_rect().get_center().y + dragged_ctrl.offset_transform_position.y
	var to_index := 0
	for c: Control in ctrls.values():
		if c == dragged_ctrl:
			continue
		if c.get_global_rect().get_center().y < offset_y:
			to_index += 1
	if to_index != dragged_index:
		var vals := (items.value as Array).duplicate()
		var moved = vals.pop_at(dragged_index)
		vals.insert(to_index, moved)
		items.value = vals
	is_dragging = false
	dragged_ctrl = null
	dragged_index = -1

func _drag_start(ctrl: Control) -> void:
	is_dragging = true
	dragged_ctrl = ctrl.find_parent("*PanelContainer*")
	dragged_ctrl.z_index = 999
	var ctrls := get_variable_value("item_refs") as Dictionary
	dragged_index = ctrls.find_key(dragged_ctrl)

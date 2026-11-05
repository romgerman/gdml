extends GDMLNode

var is_dragging := false
var dragged_ctrl: Control
var dragged_index := -1

var items: GDMLAtom = GDML.atom(&"items", [1, 2, 3, 4, 5])
var trans_in := GDML.atom(&"trans_in")
var trans_out := GDML.atom(&"trans_out")

func _enter_tree() -> void:
	var tw_in_factory = func (ctrl: Control):
		ctrl.modulate = Color.TRANSPARENT
		#ctrl.offset_transform_position.y = 50
		var tw := create_tween()
		tw.parallel().tween_property(ctrl, "modulate", Color.WHITE, 0.55).set_delay(ctrl.get_index() * 0.05)
		tw.parallel().tween_property(ctrl, "offset_transform_position:y", 0.0, 0.55)
		return tw
	
	var tw_out_factory = func (ctrl: Control):
		var tw := create_tween()
		tw.parallel().tween_property(ctrl, "modulate", Color.TRANSPARENT, 0.55)
		tw.parallel().tween_property(ctrl, "offset_transform_scale:y", 0.0, 0.55)
		tw.parallel().tween_property(ctrl, "offset_transform_position:y", 0.0, 0.55)
		return tw
	
	trans_in.value = tw_in_factory
	trans_out.value = tw_out_factory

func _ready() -> void:
	connect("item_drag_handle_input", _on_item_drag_handle_input)
	connect("add_items", func (_c):
		var vals := items.value as Array
		for i in 5:
			if vals.is_empty():
				vals.push_back(1)
			else:
				vals.push_back(vals.back() + 1)
		items.value = vals
	)
	connect("delete_item", func (c: Control):
		var vals := items.value as Array
		var index := c.get_parent().get_parent().get_index()
		vals.remove_at(index)
		items.value = vals
	)

func _process(delta: float) -> void:
	var ctrls := get_variable_value("item_refs") as Dictionary
	
	if is_dragging:
		for c in ctrls.values():
			if not is_instance_valid(c):
				continue
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
		for c in ctrls.values():
			if is_instance_valid(c):
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
	get_variable("is_trans_enabled").value = false
	dragged_ctrl.z_index = 0
	var ctrls := get_variable_value("item_refs") as Dictionary
	var offset_y := dragged_ctrl.get_global_rect().get_center().y + dragged_ctrl.offset_transform_position.y
	var to_index := 0
	for c in ctrls.values():
		if not is_instance_valid(c):
			continue
		if c == dragged_ctrl:
			continue
		if c.get_global_rect().get_center().y < offset_y:
			to_index += 1
	if to_index != dragged_index:
		var vals := (items.value as Array).duplicate()
		var moved = vals.pop_at(dragged_index)
		vals.insert(to_index, moved)
		items.value = vals
	get_variable("is_trans_enabled").value = true
	is_dragging = false
	dragged_ctrl = null
	dragged_index = -1

func _drag_start(ctrl: Control) -> void:
	is_dragging = true
	dragged_ctrl = ctrl.find_parent("*PanelContainer*")
	dragged_ctrl.z_index = 999
	var ctrls := get_variable_value("item_refs") as Dictionary
	dragged_index = ctrls.find_key(dragged_ctrl)

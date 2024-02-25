extends GridBody

var dragging := false
var dragging_offset := Vector2.ZERO

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("primary"):
		if Rect2(
			position - half_size,
			half_size * 2.0).has_point(Global.mouse_position):
			get_viewport().set_input_as_handled()
			
			velocity = Vector2.ZERO
			dragging = event.is_pressed()
			dragging_offset = Global.mouse_position - position

func _process(delta: float) -> void:
	if dragging:
		dragging = Input.is_action_pressed("primary")
		position = Global.mouse_position - dragging_offset
	else:
		velocity.y += 5.0
		move_and_slide()
		velocity.y *= pow(0.5, delta)

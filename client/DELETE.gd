extends GridBody

var dragging := false

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed(&"primary"):
		if Rect2(
			position - half_size,
			half_size * 2.0).has_point(Global.mouse_position):
			get_viewport().set_input_as_handled()
			
			dragging = event.is_pressed()

func _process(delta: float) -> void:
	if dragging:
		velocity += Global.mouse_position - position
		dragging = Input.is_action_pressed(&"primary")
	else:
		velocity.x += (Input.get_action_strength(&"right") - Input.get_action_strength(&"left")) * 5.0
	
	velocity.x = maxf(velocity.x, 0.0)
	
	velocity.y += 5.0
	move_and_slide()
	velocity *= pow(0.5, delta)

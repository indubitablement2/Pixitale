extends GridCharacterBody

const ACCELERATION = Vector2(33.0, 33.0)

func _physics_process(delta: float) -> void:
	var dir := Vector2(
		Input.get_action_strength("right") - Input.get_action_strength("left"),
		Input.get_action_strength("down") - Input.get_action_strength("up")
	)
	
	velocity += dir * ACCELERATION * delta
	velocity *= 0.7
	var blocked := move()
#	print(blocked)

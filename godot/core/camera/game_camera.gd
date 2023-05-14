extends Camera2D

var t := 0.0

func _process(delta: float) -> void:
	t += delta
	
	var dir = Vector2(
		Input.get_action_strength("right") - Input.get_action_strength("left"),
		Input.get_action_strength("down") - Input.get_action_strength("up")
	)
	
	position += dir

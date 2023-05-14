extends Camera2D

const ZOOM_MAX = Vector2(8.0, 8.0)
const ZOOM_MIN = Vector2(1.0, 1.0)

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action("camera_zoom_in"):
		zoom += Vector2(0.1, 0.1)
		zoom = zoom.clamp(ZOOM_MIN, ZOOM_MAX)
	elif  event.is_action("camera_zoom_out"):
		zoom -= Vector2(0.1, 0.1)
		zoom = zoom.clamp(ZOOM_MIN, ZOOM_MAX)

func _process(delta: float) -> void:
	var dir := Vector2(
		Input.get_action_strength("right") - Input.get_action_strength("left"),
		Input.get_action_strength("down") - Input.get_action_strength("up")
	)
	
	position += dir * delta * 100.0
	position = position.round()

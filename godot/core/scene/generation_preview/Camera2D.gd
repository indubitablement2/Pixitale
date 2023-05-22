extends Camera2D

const MIN_ZOOM = 0.05
const MAX_ZOOM = 10.0

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("camera_zoom_in"):
		zoom *= 1.5
		zoom = zoom.clamp(Vector2(MIN_ZOOM, MIN_ZOOM), Vector2(MAX_ZOOM, MAX_ZOOM))
	elif event.is_action_pressed("camera_zoom_out"):
		zoom *= 0.8
		zoom = zoom.clamp(Vector2(MIN_ZOOM, MIN_ZOOM), Vector2(MAX_ZOOM, MAX_ZOOM))
	elif event is InputEventMouseMotion && Input.is_action_pressed("camera_drag"):
		position -= event.get_relative() / zoom
	elif event.is_action_pressed("ui_home"):
		zoom = Vector2(4.0, 4.0)

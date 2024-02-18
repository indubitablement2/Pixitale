extends Node
class_name Game

## Game root.

static var node : Game

static var mouse_position := Vector2.ZERO


func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	mouse_position = GridRender.node.get_global_mouse_position()
	
	Core.queue_step_chunks(Rect2i(
		Vector2i(-1, -1),
		Vector2i(2, 2)))

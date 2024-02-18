extends Node
class_name Game

## Game root.

static var node : Game

func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	Core.queue_step_chunks(Rect2i(
		Vector2i(-1, -1),
		Vector2i(2, 2)))

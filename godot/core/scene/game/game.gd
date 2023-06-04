extends Node2D

func _process(_delta: float) -> void:
	if Input.is_action_pressed("attack"):
		var set_cell_position = Vector2i(get_global_mouse_position())
		
		Grid.set_cell_rect(Rect2i(
			set_cell_position - Vector2i(4, 4),
			Vector2i(9, 9)
			), 3)

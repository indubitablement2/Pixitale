extends Node2D

## Mouse global position.
var mouse_position := Vector2.ZERO
## Mouse grid coord. 
## Very similar to mouse_position, because 1 godot unit = 1 grid unit.
var mouse_coord := Vector2i.ZERO

## Time since engine stated. Updated once per frame.
var time := 0.0

func _process(delta: float) -> void:
	mouse_position = get_global_mouse_position()
	mouse_coord = Vector2i(mouse_position.floor())
	time += delta

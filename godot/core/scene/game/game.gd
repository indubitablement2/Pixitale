extends Node2D

@onready var m := CellMaterials.get_cell_materials_idx("")
@onready var m1 := CellMaterials.get_cell_materials_idx("rock")

var brush_size := Vector2i(33, 33)

func _process(_delta: float) -> void:
	if Input.is_action_pressed("attack"):
		var set_cell_position = Vector2i(get_global_mouse_position())
		
		Grid.set_cell_rect(Rect2i(
			set_cell_position - (brush_size - Vector2i.ONE) / 2,
			brush_size
			), m)
	elif Input.is_action_pressed("use_item_1"):
		var set_cell_position = Vector2i(get_global_mouse_position())
		
		Grid.set_cell_rect(Rect2i(
			set_cell_position - (brush_size - Vector2i.ONE) / 2,
			brush_size
			), m1)

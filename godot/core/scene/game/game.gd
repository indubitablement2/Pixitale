extends Node2D

@onready var m := CellMaterials.get_cell_materials_idx("")
@onready var m1 := CellMaterials.get_cell_materials_idx("water")
@onready var m2 := CellMaterials.get_cell_materials_idx("sand")
@onready var m3 := CellMaterials.get_cell_materials_idx("test")
@onready var m4 := CellMaterials.get_cell_materials_idx("lava")

var brush_size := 17

func _process(_delta: float) -> void:
	if Input.is_action_pressed("attack"):
		_set_rect(m)
	elif Input.is_action_pressed("use_item_1"):
		_set_rect(m1)
	elif Input.is_action_pressed("use_item_2"):
		_set_rect(m2)
	elif Input.is_action_pressed("use_item_3"):
		_set_rect(m3)
	elif Input.is_action_pressed("use_item_4"):
		_set_rect(m4)

func _set_rect(cell_material_idx : int) -> void:
	var set_cell_position = Vector2i(get_global_mouse_position())
	
	Grid.set_cell_rect(Rect2i(
		set_cell_position - (Vector2i(brush_size, brush_size) - Vector2i.ONE) / 2,
		Vector2i(brush_size, brush_size)
		), cell_material_idx)

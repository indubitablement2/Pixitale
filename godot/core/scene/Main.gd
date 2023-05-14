extends Control

func _process(_delta: float) -> void:
	var mouse_pos := get_global_mouse_position() / Grid.GRID_SCALE
	var grid_pos := Vector2i(mouse_pos)
	var mat_idx := Grid.get_cell_material_idx(grid_pos)

	$CellName.set_text(CellMaterials.cell_material_names[mat_idx])
	$Tick.set_text(str(Grid.get_tick()))
	$ChunkActive.set_text(str(Grid.is_chunk_active(Vector2i(1, 1))))
	$GridPosition.set_text(str(grid_pos))


extends GenerationPass

@export var width := 0.1
@export var y_stop := 0.5

func generate() -> void:
	var water := CellMaterials.get_cell_materials_idx("water")
	
	var ocean_top : int = WorldGeneration.get_pass("ocean").ocean_top
	
	var size := Grid.get_size()
	
	var w := int(float(size.x) * width)
	var y_end := int(float(size.y) * y_stop)
	
	# Left
	var pos := Vector2i.ZERO
	while pos.x < w:
		pos.y = ocean_top
		while pos.y < y_end:
			if Grid.get_cell_material_idx(pos) == 0:
				Generation.set_cell(pos, water)
			pos.y += 1
		pos.x += 1
	
	# Right
	pos = Vector2(size.x - w, 0)
	while pos.x < size.x:
		pos.y = ocean_top
		while pos.y < y_end:
			if Grid.get_cell_material_idx(pos) == 0:
				Generation.set_cell(pos, water)
			pos.y += 1
		pos.x += 1

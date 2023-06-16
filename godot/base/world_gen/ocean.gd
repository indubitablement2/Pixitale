extends GenerationPass

@export var ocean_size := Vector2.ZERO

func generate() -> void:
	var water := CellMaterials.get_cell_materials_idx("water")
	var sand := CellMaterials.get_cell_materials_idx("sand")
	
	var size := Grid.get_size()
	
	var ocean_width := int(ocean_size.x * float(size.x))
	
	var ocean_start := 0
	var center_x := size.x / 2
	while ocean_start < size.y:
		if Grid.get_cell_material_idx(Vector2i(center_x, ocean_start)) != 0:
			break
		ocean_start += 1
	ocean_start -= 30
	var ocean_end := ocean_start + int(ocean_size.y * float(size.y))
	
	var ipos := Vector2i.ZERO
	var fpos := Vector2.ZERO
	
	while ipos.x < ocean_width:
		ipos.y = ocean_start
		fpos.x = ipos.x
		
		while ipos.y < ocean_end:
			fpos.y = ipos.y
			
			Generation.set_cell(ipos, sand)
			
			ipos.y += 1
		
		ipos.x += 1


extends GenerationPass

@export var ocean_size := Vector2.ZERO

func generate() -> void:
	var water := CellMaterials.get_cell_materials_idx("water")
	var sand := CellMaterials.get_cell_materials_idx("sand")
	
	var size := Grid.get_size()
	
	var ipos := Vector2i.ZERO
	var fpos := Vector2.ZERO
	
	var ocean_sizei := Vector2i(Vector2(size) * ocean_size)
	
	while ipos.x < ocean_sizei.x:
		fpos.x = ipos.x
		
		while ipos.y < 1:
			fpos.y = ipos.y
			
			Generation.set_cell(ipos, sand)
			
			ipos.y += 1
		
		ipos.x += 1

extends GenerationPass

@export var ocean_size := Vector2.ZERO
@export var sand_inner_curve : Curve
@export var sand_outer_curve : Curve

@export var beach_width := 0.05

var ocean_top := 0

func generate() -> void:
	var sand := CellMaterials.get_cell_materials_idx("sand")
	
	var size := Grid.get_size()
	
	# Find where top of the ocean should be.
	@warning_ignore("integer_division")
	var center_x := size.x / 2
	ocean_top = 0
	while ocean_top < size.y:
		if Grid.get_cell_material_idx(Vector2i(center_x, ocean_top)) != 0:
			break
		ocean_top += 1
	
	var ipos := Vector2i.ZERO
	var fpos := Vector2.ZERO
	
	# Ocean depression
	var ocean_width := int(ocean_size.x * float(size.x))
	var ocean_bot := ocean_top + int(ocean_size.y * float(size.y))
	var ocean_floor := 0
	while ipos.x < ocean_width:
		fpos.x = ipos.x
		ipos.y = ocean_top
		
		var inner_end := int(float(sand_inner_curve.sample_baked(fpos.x / float(ocean_width))) * float(ocean_bot - ocean_top))
		inner_end += ocean_top
		
		if ipos.x == 0:
			ocean_floor = inner_end
		
		while ipos.y < inner_end:
			Generation.set_cell(ipos, 0)
			Generation.set_cell(Vector2i(size.x - ipos.x, ipos.y), 0)
			
			ipos.y += 1
		
		ipos.x += 1
	
	# Clear edge cells to avoid floating cells. 
	ipos = Vector2i.ZERO
	while ipos.x < 32:
		ipos.y = ocean_top - 100
		
		while ipos.y < ocean_floor:
			Generation.set_cell(ipos, 0)
			
			ipos.y += 1
		
		ipos.x += 1
	
	# Transform what isn't air into sand.
	ipos = Vector2i.ZERO
	var beach_end_x := ocean_width + int(float(size.x) * beach_width)
	while ipos.x < beach_end_x:
		ipos.y = ocean_top - 100
		fpos.x = ipos.x
		
		var inner_end := int(float(sand_outer_curve.sample_baked(fpos.x / float(beach_end_x))) * float(ocean_bot - ocean_top))
		inner_end += ocean_top
		while ipos.y < inner_end:
			fpos.y = ipos.y
			
			var cell := Grid.get_cell_material_idx(ipos)
			if cell != 0:
				Generation.set_cell(ipos, sand)
			var mirror := Vector2i(size.x - ipos.x, ipos.y)
			cell = Grid.get_cell_material_idx(mirror)
			if cell != 0:
				Generation.set_cell(mirror, sand)
			
			ipos.y += 1
		
		ipos.x += 1
	

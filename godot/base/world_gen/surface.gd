extends GenerationPass

@export var hills_height := 200.0
@export var hills_start_height := 0.05
@export var hills : FastNoiseLite

@export var rock_patches_threshold : Curve
@export var rock_patches : FastNoiseLite

func generate() -> void:
	var dirt := CellMaterials.get_cell_materials_idx("dirt")
	var rock := CellMaterials.get_cell_materials_idx("rock")
	
	hills.seed = Grid.get_seed() + 1
	rock_patches.seed = Grid.get_seed() + 2
	
	var size := Grid.get_size()
	var surface_bot := Game.cavern_start_depth
	var hills_start_offset := -int(hills_start_height * float(size.y))
	
	# Fill cavern with rock.
	Generation.set_cell_rect(Rect2i(0, surface_bot, size.x, size.y), rock)
	
	var ipos := Vector2i.ZERO
	var fpos := Vector2.ZERO
	
	while ipos.x < size.x:
		fpos.x = ipos.x
		
		ipos.y = surface_bot + hills_start_offset
		ipos.y -= int(hills.get_noise_1d(fpos.x) * hills_height)
		
		var s_step := 1.0 / float(surface_bot - ipos.y)
		var s := -s_step
		
		while ipos.y < surface_bot:
			fpos.y = ipos.y
			
			s += s_step
			if rock_patches.get_noise_2dv(fpos) > rock_patches_threshold.sample_baked(s):
				Generation.set_cell(ipos, rock)
			else:
				Generation.set_cell(ipos, dirt)
			
			ipos.y += 1
		
		ipos.x += 1




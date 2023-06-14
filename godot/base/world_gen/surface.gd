extends GenerationPass

@export var hills_height := 200.0
@export var hills_start_height := 800
@export var hills : FastNoiseLite

@export var hills_added_noise_max := 5.0
@export var hills_added_noise : FastNoiseLite

@export var hills_gradient_max := 1.0
@export var hills_strenght := 0.7
@export var hills_detail : FastNoiseLite
@export var hills_detail_strenght := 0.2
@export var hills_large : FastNoiseLite
@export var hills_large_strenght := 0.7

@export var first_rock_threshold := 1.2
@export var first_rock_frequency := 2.5
@export var first_rock_detail_strenght := 0.5
@export var first_rock_as_dirt_threshold := 0.1
@export var first_rock_as_dirt_falloff := 1.8

func generate() -> void:
	var dirt := CellMaterials.get_cell_materials_idx("dirt")
	var rock := CellMaterials.get_cell_materials_idx("rock")
	
	hills.seed = Grid.get_seed() + 1
	hills_detail.seed = Grid.get_seed() + 2
	hills_large.seed = Grid.get_seed() + 3
	
	var size := Grid.get_size()
	
	var surface_bot := GameGlobals.layer_cavern_start
	
	# Fill cavern with rock.
	Generation.set_cell_rect(Rect2i(0, surface_bot, size.x, size.y), rock)
	
	var ipos := Vector2i.ZERO
	var fpos := Vector2.ZERO
	
	while ipos.x < size.x:
		fpos.x = ipos.x
		
		ipos.y = surface_bot - hills_start_height
		ipos.y -= int(hills.get_noise_1d(fpos.x) * hills_height)
		ipos.y -= int(hills_added_noise.get_noise_1d(fpos.x) * hills_added_noise_max)
		
		while ipos.y < surface_bot:
			fpos.y = ipos.y
			
			Generation.set_cell(ipos, dirt)
			
			ipos.y += 1
		
		ipos.x += 1
	
#	while ipos.y < surface_bot:
#		ipos.x = 0
#		fpos.y = ipos.y
#
#		var hill_gradient := (float(ipos.y - hills_top) / float(hills_height)) * hills_gradient_max
#
#		while ipos.x < size.x:
#			fpos.x = ipos.x
#
#			var detail := hills_detail.get_noise_2dv(fpos)
#
#			var value := hill_gradient + (hills.get_noise_2dv(fpos) * hills_strenght + detail * hills_detail_strenght + hills_large.get_noise_2dv(fpos) * hills_large_strenght)
#
#			if value + hills.get_noise_2d(fpos.x * first_rock_frequency, fpos.y) * hills_strenght + detail * first_rock_detail_strenght > first_rock_threshold:
#				if detail / (hill_gradient * first_rock_as_dirt_falloff) > first_rock_as_dirt_threshold:
#					Grid.set_cell_generation(ipos, dirt)
#				else:
#					Grid.set_cell_generation(ipos, rock)
#			elif value > 1.0:
#				Grid.set_cell_generation(ipos, dirt)
#
#			ipos.x += 1
#
#		ipos.y += 1

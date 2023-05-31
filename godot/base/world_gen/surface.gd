extends Resource

@export var hills_top_relative := 0.2
@export var hills_height := 1000
@export var hills_gradient_strenght := 1.0
@export var hills : FastNoiseLite
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

var surface_top : int
var surface_bot : int

func pass_name() -> String:
	return "Creating surface"

func generate() -> void:
	hills.seed = Grid.get_seed() + 1
	hills_detail.seed = Grid.get_seed() + 2
	hills_large.seed = Grid.get_seed() + 3
	
	var dirt := CellMaterials.get_cell_materials_idx("dirt")
	var rock := CellMaterials.get_cell_materials_idx("rock")
	
	var size := Grid.get_size()
	
	surface_top = int(hills_top_relative * float(size.y))
	surface_bot = surface_top + hills_height
	
	var ipos := Vector2i(0, surface_bot)
	var fpos := Vector2.ZERO
	
	# Fill under surface layer with rock.
	while ipos.y < size.y:
		ipos.x = 0
		
		while ipos.x < size.x:
			Grid.set_cell_generation(ipos, rock)
			
			ipos.x += 1
		
		ipos.y += 1
	
	ipos = Vector2i(0, surface_top)
	
	# Hills
	while ipos.y < surface_bot:
		ipos.x = 0
		fpos.y = ipos.y
		
		var hill_gradient := (float(ipos.y - surface_top) / float(hills_height)) * hills_gradient_strenght
#		hill_gradient *= hill_gradient
		while ipos.x < size.x:
			fpos.x = ipos.x
			
			var detail := hills_detail.get_noise_2dv(fpos)
			
			var value := hill_gradient + (hills.get_noise_2dv(fpos) * hills_strenght + detail * hills_detail_strenght + hills_large.get_noise_2dv(fpos) * hills_large_strenght)
			
			if value + hills.get_noise_2d(fpos.x * first_rock_frequency, fpos.y) * hills_strenght + detail * first_rock_detail_strenght > first_rock_threshold:
				if detail / (hill_gradient * first_rock_as_dirt_falloff) > first_rock_as_dirt_threshold:
					Grid.set_cell_generation(ipos, dirt)
				else:
					Grid.set_cell_generation(ipos, rock)
			elif value > 1.0:
				Grid.set_cell_generation(ipos, dirt)
			
			ipos.x += 1
		
		ipos.y += 1
	
	ipos = Vector2i(0, surface_top)
	
	while ipos.y < surface_bot:
		fpos.y = ipos.y
		ipos.x = 0
		
		while ipos.x < size.x:
			fpos.x = ipos.x
			
			var v := hills_large.get_noise_2dv(fpos)
			var hue_palette_idx := int(v * float(Grid.PALETTE_IDX_MAX))
			
			v = hills_large.get_noise_2dv(fpos + Vector2(size))
			var value_palette_idx := int(v * float(Grid.PALETTE_IDX_MAX))
			
			Grid.set_cell_color(ipos, hue_palette_idx, value_palette_idx)
			
			ipos.x += 1
		
		ipos.y += 1

extends Resource

const pass_name := "Creating surface"

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

func generate() -> void:
	hills.seed = Grid.get_seed() + 1
	hills_detail.seed = Grid.get_seed() + 2
	hills_large.seed = Grid.get_seed() + 3
	
	var dirt := CellMaterials.get_cell_materials_idx("dirt")
	var rock := CellMaterials.get_cell_materials_idx("rock")
	
	var size := Grid.get_size()
	
	surface_top = int(hills_top_relative * float(size.y))
	surface_bot = surface_top + hills_height
	
	var x := 0
	var y := surface_top
	
	var xf : float
	var yf : float
	
	Grid.set_cell_rect(Rect2i(0, surface_bot, size.x, size.y), rock)
	
	# Hills
	while y < surface_bot:
		x = 0
		yf = float(y)
		
		var hill_gradient := (float(y - surface_top) / float(hills_height)) * hills_gradient_strenght
#		hill_gradient *= hill_gradient
		while x < size.x:
			xf = float(x)
			
			var detail := hills_detail.get_noise_2d(xf, yf)
			
			var value := hill_gradient + (hills.get_noise_2d(xf, yf) * hills_strenght + detail * hills_detail_strenght + hills_large.get_noise_2d(xf, yf) * hills_large_strenght)
			
			var pos := Vector2i(x, y)
			if value + hills.get_noise_2d(xf * first_rock_frequency, yf) * hills_strenght + detail * first_rock_detail_strenght > first_rock_threshold:
				if detail / (hill_gradient * first_rock_as_dirt_falloff) > first_rock_as_dirt_threshold:
					Grid.set_cell(pos, dirt)
				else:
					Grid.set_cell(pos, rock)
			elif value > 1.0:
				Grid.set_cell(pos, dirt)
			
			x += 1
		
		y += 1

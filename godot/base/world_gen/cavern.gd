extends Resource

const pass_name := "Creating caverns"

@export var horizontal_gradient : Curve
@export var vertical_gradient : Curve

@export var caverns : FastNoiseLite
@export var caverns_x_warp := 0.5

func generate() -> void:
	caverns.seed = Grid.get_seed() + 11
	
	var empty := CellMaterials.get_cell_materials_idx("")
	
	var size := Grid.get_size()
	
	var surface_pass = WorldGeneration.get_pass("Creating surface")
	var surface_top : int = surface_pass.surface_top
	
	var x := 0
	var y := surface_top
	
	var xf : float
	var yf : float
	
	while y < size.y:
		x = 0
		yf = float(y)
		
		var vg := vertical_gradient.sample_baked(yf / float(size.y))
		
		while x < size.x:
			xf = float(x)
			
			var hg := horizontal_gradient.sample_baked(xf / float(size.x))
			
			var value := caverns.get_noise_2d(xf * caverns_x_warp, yf) * vg * hg
			
			if value > 0.5:
				Grid.set_cell(Vector2i(x, y), empty)
			
			x += 1
		
		y += 1

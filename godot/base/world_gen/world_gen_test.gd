extends WorldGenerationPass

const SURFACE_START = 0.2
const CAVERN_START = 0.35

func generate() -> void:
	var nothing := CellMaterials.get_cell_materials_idx("")
	var water := CellMaterials.get_cell_materials_idx("water")
	var sand := CellMaterials.get_cell_materials_idx("sand")
	var rock := CellMaterials.get_cell_materials_idx("rock")
	var lava := CellMaterials.get_cell_materials_idx("lava")
	
	var size := Grid.get_size()
	
	var noise := FastNoiseLite.new()
	noise.noise_type = FastNoiseLite.TYPE_PERLIN
	noise.seed = Grid.get_seed()
	
	var x_start := 0
	var x_end := size.x
	var x := x_start
	
	var y_start := int(float(size.y) * SURFACE_START)
	var y_end := int(float(size.y) * CAVERN_START)
	var y := y_start
	
	while y < y_end:
		x = x_start
		# Gradient from 0.0 (SURFACE_START) to 1.0 (CAVERN_START)
		var y_gradient := float(y - y_start) / float(y_end - y_start)
		
		while x < x_end:
			var value := y_gradient
			
			if value > 0.5:
				var pos := Vector2i(x, y)
				Grid.set_cell(pos, rock)
			
			x += 1
		
		y += 1


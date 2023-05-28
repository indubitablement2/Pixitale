extends Resource

@export var horizontal_gradient : Curve
@export var vertical_gradient : Curve

@export var caverns : FastNoiseLite
@export var caverns_x_warp := 0.5

func pass_name() -> String:
	return "Creating caverns"

func generate() -> void:
	caverns.seed = Grid.get_seed() + 11
	
	var surface_pass : int = WorldGeneration.get_pass("Creating surface").surface_top
	
	Generation.cavern_pass(
		horizontal_gradient,
		vertical_gradient,
		caverns,
		caverns_x_warp,
		surface_pass
	)

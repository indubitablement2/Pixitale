extends GenerationPass

@export var horizontal_gradient : Curve
@export var vertical_gradient : Curve

@export var caverns : FastNoiseLite
@export var caverns_x_warp := 0.5
@export var caverns_threshold := 0.5

func generate() -> void:
	caverns.seed = Grid.get_seed() + 11
	
	Generation.cavern_pass(
		horizontal_gradient,
		vertical_gradient,
		caverns,
		caverns_x_warp,
		GameGlobals.layer_surface_start,
		caverns_threshold
	)

extends ModEntry

static func entry() -> void:
	print("Mod entered!")
	var passes := WorldGeneration.get_passes()
	passes.push_back(preload("res://base/world_gen/surface.tres"))
	passes.push_back(preload("res://base/world_gen/cavern.tres"))

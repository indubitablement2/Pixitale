extends ModEntry

static func entry() -> void:
	print("Mod entered!")
	WorldGeneration.get_passes().push_back(
		preload("res://base/world_gen/world_gen_test.tres")
	)

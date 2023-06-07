extends GridBiomeScanner

@onready var last_biome_idx = get_current_biome()

func _process(_delta: float) -> void:
	if scan():
		var new_biome := get_current_biome()
		print("Biome changed: ", Mod.biomes_data[last_biome_idx].id, "->", Mod.biomes_data[new_biome].id)
		last_biome_idx = new_biome

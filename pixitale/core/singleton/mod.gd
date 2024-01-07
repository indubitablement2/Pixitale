extends Node

var biomes_data : Array[BiomeData] = []

func _ready() -> void:
	_set_biomes()
	call_deferred("_mod_entry")

func get_mod_order() -> PackedStringArray:
	return ProjectSettings.get_setting(
		"mod/order",
		PackedStringArray()
	)

func set_mod_order(mod_order: PackedStringArray) -> void:
	ProjectSettings.set_setting("mod/order", mod_order)

func _set_biomes() -> void:
	for mod_name in get_mod_order():
		var entry_path := ModEntry.entry_path(mod_name)
		if FileAccess.file_exists(entry_path):
			var entry := load(entry_path) as ModEntry
			biomes_data.append_array(entry.biomes)
	
	# Add fallback biome
	biomes_data.push_back(BiomeData.new())
	
	# Apply biome priority.
	for i in biomes_data.size():
		var biome := biomes_data[i]
		
		if biome.priority_more_than.is_empty():
			continue
		
		for j in i:
			var biome2 := biomes_data[j]
			
			if biome2.id == biome.priority_more_than:
				# Swap these biomes.
				biomes_data[i] = biome2
				biomes_data[j] = biome
				break
	
	# Convert to what c++ expect.
	var new_biomes = []
	for b in biomes_data:
		new_biomes.push_back([b.min_cell_count, b.min_depth, b.min_distance_from_center])
	
	Grid.set_biomes(new_biomes)

func _mod_entry() -> void:
	for mod_name in get_mod_order():
		var entry_path := ModEntry.entry_path(mod_name)
		if FileAccess.file_exists(entry_path):
			var entry := load(entry_path) as ModEntry
			entry.entry()
	
	# Add core generation passes.
	var passes := WorldGeneration.get_passes()
	passes.push_back(preload("res://core/world_gen/post_generation.tres"))
	passes.push_back(preload("res://core/world_gen/simulation.tres"))

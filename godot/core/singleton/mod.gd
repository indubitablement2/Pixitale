extends Node

func _ready() -> void:
	call_deferred("_mod_entry")

func get_mod_order() -> PackedStringArray:
	return ProjectSettings.get_setting(
		"mod/order",
		PackedStringArray()
	)

func set_mod_order(mod_order: PackedStringArray) -> void:
	ProjectSettings.set_setting("mod/order", mod_order)

func _mod_entry() -> void:
	for mod_name in get_mod_order():
		var entry_path := ModEntry.entry_path(mod_name)
		if FileAccess.file_exists(entry_path):
			var entry := load(entry_path)
			entry.entry()
	
	# Add core generation passes.
	var passes := WorldGeneration.get_passes()
	passes.push_back(preload("res://core/world_gen/post_generation.tres"))
	passes.push_back(preload("res://core/world_gen/simulation.tres"))

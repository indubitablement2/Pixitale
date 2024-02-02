extends Resource
class_name ModEntry
#
#@export var passes : Array[GenerationPass] = []
#
### Order is priority.
#@export var biomes : Array[BiomeData] = []
#
#static func entry_path(mod_name: String) -> String:
	#return "res://" + mod_name + "/entry.tres"
#
## This will be called when loading this mod
## if it is located at "res://[mod_name]/entry.gd".
#func entry() -> void:
	#var world_passes := WorldGeneration.get_passes()
	#for p in passes:
		#world_passes.push_back(p)

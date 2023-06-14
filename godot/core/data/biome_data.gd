@tool
extends Resource
class_name BiomeData

## An unique identifier. Used on cell data.
@export var id := ""

@export_category("Background")
@export var backgrounds : Array[Texture2D] = []

@export_category("Condition")
## This biome requite that this many cells nearby are tagged as this biome.
@export var min_cell_count := 0

@export_enum("anywhere", "surface_and_lower", "cavern_and_lower", "hell_and_lower") var min_depth_preset := "" : set = set_min_depth_preset
## 0 is the grid highest point, 0 is the lowest.
## 0 means this biome can appear from 0 (top of the grid) to +infinity 
@export var min_depth := 0.0
## Relative distance from center.
## 0.0 means this biome can appear anywhere
## 0.5 means from half of the world to +/-infinity on both sides.
@export var min_distance_from_center := 0.0

@export_category("Priority")
## This move the biome on the priority list just before priority_more_than.
## Leave to an empty String to not more.
@export var priority_more_than : String = ""

func set_min_depth_preset(value: String) -> void:
	min_depth_preset = value
	
	match value:
		"anywhere":
			min_depth = -999999.0
		"surface_and_lower":
			min_depth = GameGlobals.SURFACE_START
		"cavern_and_lower":
			min_depth = GameGlobals.CAVERN_START
		"hell_and_lower":
			min_depth = GameGlobals.HELL_START

 

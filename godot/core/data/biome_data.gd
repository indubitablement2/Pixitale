extends Resource
class_name BiomeData

## An unique identifier. Used on cell data.
@export var id := ""

@export_category("Background")
## Root node should inherit from BiomeBackground.
@export var background : PackedScene
@export var cavern_background : Texture2D

@export_category("Condition")
## This biome requite that at least
# this many cells nearby are tagged as this biome.
@export var min_cell_count := 0

@export_enum("anywhere", "cavern_and_lower", "custom") var min_depth_preset := "anywhere"
## 0 is the grid highest point, 0 is the lowest.
## 0 means this biome can appear from 0 (top of the grid) to +infinity 
@export var min_depth := 0.0 : get = get_min_depth
## Relative distance from center.
## 0.0 means this biome can appear anywhere
## 0.5 means from half of the world to +/-infinity on both sides.
@export var min_distance_from_center := 0.0

@export_category("Priority")
## This move the biome on the priority list just before priority_more_than.
## Leave to an empty String to not move.
@export var priority_more_than : String = ""

func get_min_depth() -> float:
	match min_depth_preset:
		"anywhere":
			return -999999.0
		"cavern_and_lower":
			return GameGlobals.CAVERN_START
		_:
			return min_depth

 

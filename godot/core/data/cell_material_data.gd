extends Resource
class_name CellMaterialData

# Default values are what empty cell (id: "", idx: 0) uses.

@export var display_name := ""

@export var id := ""
## tag "all" is implicit here.
@export var tags := PackedStringArray() 

@export_category("Render")
@export var base_color := Color.TRANSPARENT
## If this cell can have hue/value modifier.
@export var can_color := false
## Alpha not used.
@export var glow := Color.BLACK
## Alpha blocks light. Rgb tint light passing through.
@export var light_block := Color.TRANSPARENT

## 0 == no noise.
## Generate noise from min to max (including max)
@export_range(0, 15) var noise_max := 0
## Only use red channel. 
## Compressed to 4bits [0..15]
## Can be null, if not needed.
@export var values :Texture2D = null

@export_category("Movement")
## Can swap position with less dense cell.
@export var density := 0
## A low chance to replace the cells with empty
## after moving (or being moved by another cell) in horizontal direction.
## This is to prevent infinite horizontal movement back and forth.
@export var liquid_movement_disapear_chance := 0.0
## 0: no sand movement.
## else: sand movement every x tick.
@export var sand_movement := 1
## 0: no horizontal movement.
## else: hotizontal movement every x tick.
@export var liquid_movement := 1

@export_category("Interaction")
@export var durability := 0
## Which biome this cell count toward.
## Leave to an empty String for none.
@export var biome_id := ""

@export_category("Collision")
@export var collision_type := Grid.COLLISION_NONE
## Less than 1 is slippery, more than 1 is sticky.
@export_range(0.0, 2.0, 0.01, "or_greater") var friction := 1.0
#@export_range(0.0, 0.9, 0.01) var bounciness := 0.0

@export_category("Ignite Preset")
@export var use_fire_preset := false
## Each level higher than `ignite_temperature_needed`
## will have 2.5x the chance to ignite.
## With ignite_temperature_needed set to Amber and  ignite_chance to 0.05:
## Amber: 0.1, Fire: 0.25, RagingFire: 0.625
@export var ignite_chance := 0.1
@export_enum("Amber", "Fire", "RagingFire") var ignite_temperature_needed := 0
@export_enum("Air", "Amber") var ignite_into_preset :String
@export var ignite_into_custom :String

@export_category("Event")
# Chance/do what?
@export var on_destroyed := false

var num_reaction := 0
## [[probability: int, out1: int, out2: int]]
var reactions := []

# Converted biome id to biome idx.
var biome_idx := 0

func _to_string() -> String:
	return id + " - " + display_name
	

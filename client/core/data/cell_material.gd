extends Node
class_name CellMaterial

## Default values are what empty cell (always idx 0) uses.
## CellMaterial can not be added, removed or modified while in-game.

@export var display_name := ""

## Used in CellReaction to refer to a group of CellMaterial.
@export var tags : Array[StringName] = [&"all"]

@export_category("Render")
## Static base color based on coordinates.
## Keep as null to use base_color instead.
@export var base_color_image : Image = null
@export var base_color := Color.TRANSPARENT

## Permanent glow/bloom. Alpha is not used.
@export var glow := Color.BLACK
## Light passing through this cell is blocked by alpha and tinted by rgb.
@export var light_modulate := Color.TRANSPARENT

## If this cell can be colored.
@export var can_color := false
## Set color of new cell based on coordinates to match this image.
## Can be null, if not needed.
@export var new_color_image : Image = null
## Use noise to randomly darken color up to this amount.
## Keep as 0 to add no noise.
@export_range(0.0, 1.0) var new_darken_noise_max := 0.0

@export_category("Movement")
## Can swap position with less dense cell.
@export var density := 0
## 0: no movement.
## 1: vertical movement down.
## -1: vertical movement up.
@export_enum("no movement:0", "up:-1", "down:1") var movement_vertical_step := 0
## >= 1.0: can always move.
## < 1.0: may randomly stop moving.
## This is to simulate slow moving cell.
@export var movement_chance := 1.0
## Needs movement_vertical_step. 
## Vertical movement is tried first.
@export var horizontal_movement := false

@export_category("Interaction")
@export var durability := 0
## Which biome this cell count toward.
## Leave to an empty String for none.
@export var biome_id := &""

@export_category("Collision")
@export var collision_type := 0 #CellMaterial.COLLISION_NONE
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

var idx := 0

var num_reaction := 0
## [[probability: int, out1: int, out2: int]]
var reactions := []

# Converted biome id to biome idx.
var biome_idx := 0


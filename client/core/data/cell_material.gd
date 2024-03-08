extends Node
class_name CellMaterial

## Default values are what empty cell (always idx 0) uses.
## CellMaterial can not be added or removed while in-game.
## Some property can be modified while in-game.

@export var display_name := ""

## Used in CellReaction to refer to a group of CellMaterial.
## Name use PascalCase. Tags use snake_case.
@export var tags : Array[StringName] = [&"all"]

@export_category("Render")
## Base color based on coordinates.
## Keep as null to use base_color instead.
## Can be modified while in-game, 
## but size has to stay the same and can not change to/from null.
## If modifying the Image only,
## call set_base_color_image for changes to take effect.
@export var base_color_image : Image = null
func set_base_color_image(value: Image) -> void:
	base_color_image = value
	if idx != -1:
		GridApi.base_color_atlas.get_img_mut().blit_rect(
			base_color_image,
			Rect2i(Vector2i.ZERO, base_color_image.get_size()),
			base_color_atlas_coord)
## Flat base color. Has no effect if base_color_image is used.
## Can be modified while in-game, if base_color_image is null.
@export var base_color := Color.TRANSPARENT : set = set_base_color
func set_base_color(value: Color) -> void:
	base_color = value
	if idx != -1:
		GridApi.base_color_atlas.get_img_mut().set_pixelv(
			base_color_atlas_coord,
			base_color)
var base_color_atlas_coord := Vector2i.ZERO

## Permanent glow/bloom. Alpha is not used.
## Can be modified while in-game.
@export var glow := Color.BLACK : set = set_glow
func set_glow(value: Color) -> void:
	glow = value
	if idx != -1:
		GridApi.glow.get_img_mut().set_pixel(idx, 0, glow)
	
## Light passing through this cell is blocked by alpha and tinted by rgb.
## Can be modified while in-game.
@export var light_modulate := Color.TRANSPARENT : set = set_light_modulate
func set_light_modulate(value: Color) -> void:
	light_modulate = value
	if idx != -1:
		GridApi.light_modulate.get_img_mut().set_pixel(idx, 0, light_modulate)

## If this cell can be colored.
@export var can_color := false

@export_category("Movement")
## Can swap position with less dense cell.
@export var density := 0

## How many vertical movement per step.
## Negative value moves upwards.
## For solid, should be 0 (never move vertically).
@export_range(-16, 16) var vertical_movement := 0

## How many horizontally movement per step.
## A cell start moving horizontally after moving vertically at least once
## or from horizontal_movement_start_chance.
@export_range(1, 16) var horizontal_movement = 1;
## Chance to spontaneously start moving horizontally when active.
## Does not keep cell active if chance fail.
@export_range(0.0, 1.0) var horizontal_movement_start_chance = 0.0;
## Chance to stop moving horizontally.
@export_range(0.0, 1.0) var horizontal_movement_stop_chance = 1.0;

## Remove this cell on blocked horizontal movement when moving atop inactive cells.
## This is for top layer of fluid to eventually become inactive,
## instead of moving back and forth forever.
@export_range(0.0, 1.0) var dissipate_on_horizontal_blocked_chance = 0.0;

## When blocked from moving horizontally, 
## try to reverse direction instead of stopping.
@export var can_reverse_horizontal_movement := false

@export_category("Interaction")
#@export var durability := 0

### Which biome this cell count toward.
### Leave to an empty String for none.
#@export var biome_id := &""

@export var collision_type := Grid.CELL_COLLISION_NONE
## Less than 1 is slippery, more than 1 is sticky.
#@export_range(0.0, 2.0, 0.01, "or_greater") var friction := 1.0
##@export_range(0.0, 0.9, 0.01) var bounciness := 0.0

var idx := -1

extends Node

enum WORLD_SIZE {
	LARGE,
	MEDIUM,
	SMALL,
	TINY
}
# 2.1gb
const WORLD_SIZE_LARGE := Vector2i(32768, 16384)
# 0.9gb
const WORLD_SIZE_MEDIUM := Vector2i(21504, 10240)
# 0.5gb
const WORLD_SIZE_SMALL := Vector2i(16384, 8192)
# 0.2gb
const WORLD_SIZE_TINY := Vector2i(10240, 5120)

var world_size := WORLD_SIZE.TINY

func get_wish_world_size() -> Vector2i:
	match world_size:
		WORLD_SIZE.LARGE:
			return WORLD_SIZE_LARGE
		WORLD_SIZE.MEDIUM:
			return WORLD_SIZE_MEDIUM
		WORLD_SIZE.SMALL:
			return WORLD_SIZE_SMALL
		_:
			return WORLD_SIZE_TINY

const SURFACE_START := 0.05
const CAVERN_START := 0.3
const HELL_START := 0.9

var layer_surface_start := 0
var layer_cavern_start := 0
var layer_hell_start := 0

func compute_layers_starts() -> void:
	var hf := float(Grid.get_size().y)
	layer_surface_start = int(hf * SURFACE_START)
	layer_cavern_start = int(hf * CAVERN_START)
	layer_hell_start = int(hf * HELL_START)

var background_offset := Vector2.ZERO

func update_background_y_offset() -> void:
	match world_size:
		WORLD_SIZE.LARGE:
			background_offset.y = layer_cavern_start * 2.14
		WORLD_SIZE.MEDIUM:
			background_offset.y = layer_cavern_start * 1.83
		WORLD_SIZE.SMALL:
			background_offset.y = layer_cavern_start * 1.62
		_:
			background_offset.y = layer_cavern_start


var player_position := Vector2.ZERO
var player_alive := false



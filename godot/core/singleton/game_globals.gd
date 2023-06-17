extends Node

enum WORLD_SIZE_PRESET {
	LARGE,
	MEDIUM,
	SMALL,
	TINY,
	CUSTOM
}
# 2.1gb
const WORLD_SIZE_LARGE := Vector2i(32768, 16384)
# 0.9gb
const WORLD_SIZE_MEDIUM := Vector2i(21504, 10240)
# 0.5gb
const WORLD_SIZE_SMALL := Vector2i(16384, 8192)
# 0.2gb
const WORLD_SIZE_TINY := Vector2i(10240, 5120)

func get_world_size_from_preset(preset: WORLD_SIZE_PRESET, custom: Vector2i) -> Vector2i:
	match preset:
		WORLD_SIZE_PRESET.LARGE:
			return WORLD_SIZE_LARGE
		WORLD_SIZE_PRESET.MEDIUM:
			return WORLD_SIZE_MEDIUM
		WORLD_SIZE_PRESET.SMALL:
			return WORLD_SIZE_SMALL
		WORLD_SIZE_PRESET.TINY:
			return WORLD_SIZE_TINY
		_:
			return custom

const SURFACE_START := 0.3
const CAVERN_START := 0.4
const HELL_START := 0.9

var layer_surface_start := 0
var layer_cavern_start := 0
var layer_hell_start := 0

func compute_layers_starts() -> void:
	var hf := float(Grid.get_size().y)
	layer_surface_start = int(hf * SURFACE_START)
	layer_cavern_start = int(hf * CAVERN_START)
	layer_hell_start = int(hf * HELL_START)

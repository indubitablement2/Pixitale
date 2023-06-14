extends Node

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

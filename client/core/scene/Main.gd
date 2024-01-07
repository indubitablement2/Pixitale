extends Control

var t := 0

@export var wish_size := Game.WORLD_SIZE.TINY

func _ready() -> void:
	WorldGeneration.generation_finished.connect(_on_generation_finished, 1)
	
	randomize()
	WorldGeneration.call_deferred("generate_world", wish_size, randi())

func _on_generation_finished(_canceled: bool) -> void:
	Background.start()
	process_mode = Node.PROCESS_MODE_INHERIT
	
	hide()
#	Game.game_ready = true
	GridRender.set_enabled(true)

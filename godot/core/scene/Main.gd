extends Control

var t := 0

@export var wish_size := GameGlobals.WORLD_SIZE.TINY

func _ready() -> void:
	WorldGeneration.generation_finished.connect(_on_generation_finished, 1)
	
	randomize()
	WorldGeneration.call_deferred("generate_world", wish_size, randi())

func _on_generation_finished(_canceled: bool) -> void:
	Background.start()
	process_mode = Node.PROCESS_MODE_INHERIT

func _process(_delta: float) -> void:
	Grid.step()


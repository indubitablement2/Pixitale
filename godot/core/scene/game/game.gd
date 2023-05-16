extends Node2D

func _ready() -> void:
	Grid.new_empty(512, 256)

func _process(_delta: float) -> void:
	Grid.step_manual()

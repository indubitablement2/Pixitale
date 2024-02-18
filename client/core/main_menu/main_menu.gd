extends Control
class_name MainMenu

const game_scene := preload("res://core/game/game.tscn")

func _ready() -> void:
	GridApi.load_mods()
	
	queue_free()
	
	get_parent().call_deferred(&"add_child", game_scene.instantiate())


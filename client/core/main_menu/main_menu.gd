extends Control
class_name MainMenu

const game_scene := preload("res://core/game/game.tscn")

func _ready() -> void:
	GridApi.load_mods()
	Core.queue_step_chunks(Rect2i(
		Vector2i(-1, -1),
		Vector2i(2, 2)))
	queue_free()
	
	get_parent().call_deferred(&"add_child", game_scene.instantiate())


extends Control
class_name MainMenu

const game_scene := preload("res://core/game/game.tscn")

func _on_new_pressed() -> void:
	var peer := WebSocketMultiplayerPeer.new()
	var err := peer.create_server(27939, "127.0.0.1")
	if err:
		printerr(error_string(err))
		return
	
	GridApi.load_mods()
	
	var game := game_scene.instantiate()
	get_parent().add_child(game)
	game.multiplayer.multiplayer_peer = peer
	
	queue_free()

func _on_join_pressed() -> void:
	var peer := WebSocketMultiplayerPeer.new()
	var err := peer.create_client("127.0.0.1:27939")
	if err:
		printerr(error_string(err))
		return
	else:
		print_debug("ok")
	
	GridApi.load_mods()
	
	var game := game_scene.instantiate()
	get_parent().add_child(game)
	game.multiplayer.multiplayer_peer = peer
	
	queue_free()


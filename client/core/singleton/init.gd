extends Node

func _ready() -> void:
	# Root viewport only display `Visible` items.
	get_parent().set_canvas_cull_mask(1)
	
	if PixitaleTests.assert_enabled():
		PixitaleTests.run_tests()

func _exit_tree() -> void:
	print("Exiting...")

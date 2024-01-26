extends Node


func _ready() -> void:
	if PixitaleTests.assert_enabled():
		PixitaleTests.run_tests()


func _exit_tree() -> void:
	#Grid.free_memory()
	print("Exiting...")



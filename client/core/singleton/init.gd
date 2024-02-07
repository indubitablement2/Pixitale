extends Node


func _ready() -> void:
	if PixitaleTests.assert_enabled():
		PixitaleTests.run_tests()
	
	GridApi.load_mods()


func _exit_tree() -> void:
	Grid.clear()
	print("Exiting...")

static func asd(who: String) -> void:
	print(who)

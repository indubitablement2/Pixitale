extends Node

func _ready() -> void:
	if PixitaleTests.assert_enabled():
		PixitaleTests.run_tests()

func _exit_tree() -> void:
	print("Exiting...")

static func asd(who: String) -> void:
	print(who)
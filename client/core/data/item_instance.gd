extends RefCounted
class_name ItemInstance

## Meant as the base class of items.

@export var data : ItemData
@export var quantity := 1

# 'Virtual' functions:

func use(_character: Node2D, _at: Vector2) -> void:
	return

#func wield(player) -> void:
	#pass

#func equip(player) -> void:
	#pass

#func unequip(player) -> void:
	#pass

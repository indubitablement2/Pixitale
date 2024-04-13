extends Resource
class_name ItemData

@export var max_quantity := 1

@export var icon : Texture2D
@export var display_name := ""

## If true, Player's arm isn't animated when item is weilded.
@export var control_arm_while_weilded := false
## Scene instanciated and added to Player's hand when item is weilded.
## Can stay null.
## Node is free when item is not weilded anymore.
@export var weild_scene : PackedScene = null

## Scene is instanciated and added to Player when item is equiped.
## Keep null if item is not equipable. 
## Node is free when item is not equiped anymore.
@export var equip_scene : PackedScene = null

## Scene is instanciated and added to Player when item is used.
## Keep null if item is not usable.
## Scene has to free itself.
## Prevent using again until scene is free.
@export var use_scene : PackedScene = null

func make_instance(qty := 1) -> ItemInstance:
	var item := ItemInstance.new()
	item.data = self
	item.quantity = mini(qty, max_quantity)
	return item

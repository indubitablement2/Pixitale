extends Resource
class_name ItemData

@export var max_quantity := 1
@export var _item_script : GDScript

@export var icon : Texture2D
@export var display_name := ""

func make_instance(qty := 1) -> ItemInstance:
	var item : ItemInstance = _item_script.new()
	item.data = self
	item.quantity = qty
	return item

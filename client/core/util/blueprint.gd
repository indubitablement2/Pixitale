extends Resource
class_name Blueprint

@export var material_idx: PackedByteArray
@export var color_idx: PackedByteArray
@export var background_material_idx: PackedByteArray
@export var background_color_idx: PackedByteArray
@export var size: Vector2i

static func take(rect: Rect2i, background: bool) -> Blueprint:
	var data : Array[int] = []
	data.resize(rect.get_area())
	var xl := false
	
	var iter := Grid.iter_rect(rect)
	var i := 0
	while iter.next():
		var mat_idx := iter.get_material_idx()
		xl = xl || (mat_idx > 255)
		data[i] = mat_idx
	
	if background:
		pass
	
	var bp := Blueprint.new()
	
	return bp


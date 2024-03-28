extends Resource
class_name Blueprint

@export var foreground: Image
@export var background: Image = null

static func take(rect: Rect2i, take_background: bool) -> Blueprint:
	var bp := Blueprint.new()
	bp.foreground = Grid.get_cell_buffer(rect, false, true)
	if take_background:
		bp.background = Grid.get_cell_buffer(rect, true, true)
	return bp

func get_material_idx(local_coord: Vector2i) -> int:
	return Grid.color_to_material_idx(foreground.get_pixelv(local_coord))

func get_color_idx(local_coord: Vector2i) -> int:
	return Grid.color_to_color_idx(foreground.get_pixelv(local_coord))

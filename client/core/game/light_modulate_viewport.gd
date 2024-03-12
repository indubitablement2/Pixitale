extends SubViewport

func _process(_delta: float) -> void:
	size = GridRender.node.raw_cell_rect.size

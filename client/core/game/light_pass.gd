extends SubViewport
class_name LightPass

## This is the place to add light so that they are affected by bloom.

static var node : LightPass

@onready var cell_light_sprite : Sprite2D = $CellLight

func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	size = GridRender.node.raw_cell_rect.size
	canvas_transform.origin = -GridRender.node.position
	cell_light_sprite.position = GridRender.node.position

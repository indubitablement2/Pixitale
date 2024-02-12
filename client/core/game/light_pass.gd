extends SubViewport
class_name LightPass

## This is the place to add light so that they are affected by bloom.

static var node : LightPass

func _init() -> void:
	node = self

func _ready() -> void:
	$CellLight.set_texture(GridRender.node.raw_cell_texture)

func _process(_delta: float) -> void:
	size = GridRender.node.raw_cell_rect.size
	canvas_transform.origin = -GridRender.node.position
	#cell_light_sprite.material.set_shader_parameter(
		#&"global_light_local_end",
		#Game.cavern_start_depth - render_origin.y
	#)

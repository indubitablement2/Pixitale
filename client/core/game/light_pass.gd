extends SubViewport
class_name LightPass

## This is the place to add light so that they are affected by bloom.

static var node : LightPass

## Raw cell data to light modulate.
@export var background_light_modulate : SubViewport
## Raw cell data to light modulate.
@export var foreground_light_modulate : SubViewport

@export var environment_light :Sprite2D
@export var environment_light_ray : Sprite2D
@export var foreground_glow :Sprite2D

func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	size = GridRender.node.raw_cell_rect.size
	canvas_transform.origin = -GridRender.node.position
	
	environment_light.position = GridRender.node.position
	environment_light_ray.position = GridRender.node.position
	foreground_glow.position = GridRender.node.position

func set_environment_light_color(col: Color) -> void:
	environment_light.material.set_shader_parameter(&"light_color", col)

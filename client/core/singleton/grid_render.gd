extends Node2D

const MAX_RENDER_SIZE = 2048

var view_origin := Vector2.ZERO
var view_size := Vector2.ZERO

# Render light beyong what the screen can see, so that
# light does not pop-in.
var render_padding := Vector2i(64, 64)

var render_origin := Vector2i.ZERO
var render_size := Vector2i.ZERO

@onready var data_texture := ImageTexture.new()

@onready var light_viewport : SubViewport = $LightPass
@onready var light_global_light_sprite : Sprite2D = $LightPass/GlobalLight

@onready var cell_sprite : Sprite2D = $Cell
@onready var light_sprite : Sprite2D = $Light

func _ready() -> void:
	cell_sprite.texture = data_texture
	light_global_light_sprite.texture = data_texture
	set_enabled(false)

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	view_origin = -ctrans.get_origin() / ctrans.get_scale()
	view_size = get_viewport_rect().size / ctrans.get_scale()
	
	render_origin = Vector2i(view_origin.floor()) - render_padding
	render_size = Vector2i(view_size.floor()) + render_padding * 2
	
	if render_size.x > MAX_RENDER_SIZE:
		push_error("Render size too high", render_size)
		return
	
	var data_img := Grid.get_cell_data(
		render_size,
		Rect2i(render_origin, render_size)
	)
	
	if render_size != Vector2i(data_texture.get_size()):
		data_texture.set_image(data_img)
		print("new data texture size: ", render_size)
	else:
		data_texture.update(data_img)
	
	position = render_origin
	
	light_viewport.size = render_size
	light_global_light_sprite.material.set_shader_parameter(
		&"global_light_local_end",
		Game.cavern_start_depth - render_origin.y
	)

func set_enabled(enabled: bool) -> void:
	set_process(enabled)
	set_visible(enabled)


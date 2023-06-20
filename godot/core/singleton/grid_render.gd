extends Node2D

const MAX_RENDER_SIZE = 2048

# Render light beyong what the screen can see, so that
# light does not pop-in.
var render_padding := Vector2i(50, 50) : set = set_render_padding

# Add some border cells beyond what is rendered.
# Used for light raycast calculation.
var data_padding := Vector2i(64, 64) : set = set_data_padding

var data_origin := Vector2i.ZERO
var data_size := Vector2i.ZERO
# Size could be bigger than data_size 
# as it also include pixels that are kept to avoid resizing texture
# but are not updated.
@onready var data_texture := ImageTexture.new()

var render_origin := Vector2i.ZERO
var render_size := Vector2i.ZERO

var view_origin := Vector2.ZERO
var view_size := Vector2.ZERO

@onready var color_viewport : SubViewport = $ColorPass
@onready var color_data_sprite : Sprite2D = $ColorPass/Data

@onready var light_viewport : SubViewport = $LightPass
@onready var light_color_sprite : Sprite2D = $LightPass/Color

@onready var color_sprite : Sprite2D = $Color
@onready var light_spirte : Sprite2D = $Light

func _ready() -> void:
	set_data_padding(data_padding)
	
	color_data_sprite.texture = data_texture

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	view_origin = -ctrans.get_origin() / ctrans.get_scale()
	view_size = get_viewport_rect().size / ctrans.get_scale()
	
	render_origin = Vector2i(view_origin.floor()) - render_padding
	render_size = Vector2i(view_size.floor()) + render_padding * 2
	
	if render_size.x > MAX_RENDER_SIZE:
		push_error("Render size too high", render_size)
		return
	
	data_origin = render_origin - data_padding
	data_size = render_size + data_padding * 2
	
	var resized := false
	var data_texture_size := Vector2i(data_texture.get_size())
	var min_data_texture_size := Vector2i(
		nearest_po2(data_size.x),
		nearest_po2(data_size.y)
	)
	if min_data_texture_size != data_texture_size:
		var max_data_texture_size := min_data_texture_size * 2
		if min_data_texture_size.x > data_texture_size.x || min_data_texture_size.y > data_texture_size.y || data_texture_size.x > max_data_texture_size.x || data_texture_size.y > max_data_texture_size.y:
			data_texture_size = min_data_texture_size
			print("new data texture size: ", data_texture_size)
			resized = true
	
	var data_img := Grid.get_cell_data(
		data_texture_size,
		Rect2i(data_origin, data_size)
	)
	if resized:
		data_texture.set_image(data_img)
	else:
		data_texture.update(data_img)
	
	color_viewport.size = data_size
	light_viewport.size = render_size
	
	position = render_origin
	
	light_color_sprite.material.set_shader_parameter(
		&"global_origin",
		position
	)

func set_enabled(enabled: bool) -> void:
	set_process(enabled)
	set_visible(enabled)

func set_render_padding(value: Vector2i) -> void:
	render_padding = value

func set_data_padding(value: Vector2i) -> void:
	data_padding = value
	
	light_color_sprite.position = -data_padding
	light_color_sprite.material.set_shader_parameter(
		&"color_offset",
		Vector2(data_padding)
	)
	
	color_sprite.position = -data_padding


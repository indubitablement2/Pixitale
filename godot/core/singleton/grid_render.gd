extends Sprite2D

const MAX_DATA_SIZE = Vector2i(2048, 2048)

# Add some border cells beyond what the screen can see.
# Used for light raycast calculation.
var data_padding := Vector2(64.0, 64.0)

var data_origin := Vector2i.ZERO
var data_size := Vector2i.ZERO
# Size could be bigger than data_size 
# as it also include pixels that are kept to avoid resizing texture
# but are not updated.
@onready var data_texture := ImageTexture.new()

@onready var color_sprite : Sprite2D = $ColorPass/Sprite2D
@onready var color_viewport : SubViewport = $ColorPass
@onready var color_texture := color_viewport.get_texture()

func _ready() -> void:
	color_sprite.texture = data_texture
	texture = color_viewport.get_texture()

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	var view_origin := -ctrans.get_origin() / ctrans.get_scale()
	var view_size := get_viewport_rect().size / ctrans.get_scale()
	
#	render_size = Vector2i(view_size.floor()) + Vector2i.ONE
#	render_origin = Vector2i(view_origin.floor())
	
	data_origin = Vector2i((view_origin - data_padding).floor())
	data_size = Vector2i((view_size + data_padding * 2.0).floor()) + Vector2i.ONE
	data_size = data_size.clamp(Vector2i(2, 2), MAX_DATA_SIZE)
	
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
	
	# TODO: Hysteresis
	color_viewport.size = data_size
	
	position = data_origin

func set_enabled(enabled: bool) -> void:
	set_process(enabled)
	set_visible(enabled)


extends Sprite2D
class_name  GridSprite

var draw_position := Vector2i.ZERO
var draw_size := Vector2i(1, 1) : set = _set_draw_size
var follow_view := true

var _resized := true
var _image_size := Vector2i.ZERO

@onready var _tex := ImageTexture.new()

func _ready() -> void:
	texture = _tex

func _process(_delta: float) -> void:
	if follow_view:
		_follow_view()
	
	var img := Grid.get_cell_data(_image_size, Rect2i(draw_position, draw_size))
	
	if _resized:
		_tex.set_image(img)
		_resized = false
	else:
		_tex.update(img)

func _resize(new_size: Vector2i) -> void:
	new_size = Vector2i(
		nearest_po2(new_size.x),
		nearest_po2(new_size.y)
	)
	
	if new_size != _image_size:
		_image_size = new_size
		_resized = true
		print("new img size: ", _image_size)

func _follow_view() -> void:
	var ctrans := get_canvas_transform()
	var view_origin := -ctrans.get_origin() / ctrans.get_scale()
	var view_size := get_viewport_rect().size / ctrans.get_scale()
	
	draw_position = view_origin
	_set_draw_size(view_size + Vector2.ONE)
	
	position = view_origin

func _set_draw_size(value: Vector2i) -> void:
	if value == draw_size:
		return
	
	draw_size = value
	_resize(draw_size)


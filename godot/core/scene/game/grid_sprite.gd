extends Sprite2D
class_name  GridSprite

var draw_size := Vector2i(1, 1) : set = _set_draw_size
var draw_position := Vector2i.ZERO : set = _set_draw_position
var _resized := true

@onready var _img := Image.create(1, 1, false, Image.FORMAT_RF)
@onready var _tex := ImageTexture.create_from_image(_img)

func _ready() -> void:
	texture = _tex

func _process(_delta: float) -> void:
	Grid.update_image_data(_img, Rect2i(draw_position, draw_size))
	
	if _resized:
		_tex.set_image(_img)
		_resized = false
	else:
		_tex.update(_img)

func _resize(new_size: Vector2i) -> void:
	_img = Image.create(
		nearest_po2(new_size.x),
		nearest_po2(new_size.y),
		false,
		Image.FORMAT_RF
	)
	
	_resized = true
	
	print("new img size: ", _img.get_size())

#func _view_rect() -> Rect2i:
#	var ctrans := get_canvas_transform()
#	var view_origin := -ctrans.get_origin() / ctrans.get_scale()
#	var view_size := get_viewport_rect().size / ctrans.get_scale()
#
#	return Rect2i(view_origin, view_size + Vector2.ONE)

func _set_draw_size(value: Vector2i) -> void:
	if value == draw_size:
		return
	
	draw_size = value
	
	var img_size := _img.get_size()
	if img_size.x < draw_size.x || img_size.y < draw_size.y:
		_resize(draw_size)

func _set_draw_position(value: Vector2i) -> void:
	draw_position = value
	position = draw_position



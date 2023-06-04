extends Sprite2D
class_name  GridSprite

var sun_ray_origin := Vector2(-64.0, -64.0)

# draw + some padding.
var fetch_origin := Vector2i.ZERO
var fetch_size := Vector2i.ZERO

# Cells which will be shown.
var draw_origin := Vector2i.ZERO
var draw_size := Vector2i.ZERO

var _image_size := Vector2i.ZERO
@onready var _tex := ImageTexture.new()

func _ready() -> void:
	texture = _tex

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	var view_origin := -ctrans.get_origin() / ctrans.get_scale()
	var view_size := get_viewport_rect().size / ctrans.get_scale()
	
	# Add some border cells beyond what the screen can see.
	# Ray light will need these.
	var padding_sub := Vector2(minf(sun_ray_origin.x, 0.0), minf(sun_ray_origin.y, 0.0))
	var padding_add := Vector2(maxf(sun_ray_origin.x, 0.0), maxf(sun_ray_origin.y, 0.0))
	fetch_origin = Vector2i((view_origin + padding_sub).floor())
	fetch_size = Vector2i(
		(view_size + padding_sub.abs() * 2.0 + padding_add).floor()
	) + Vector2i.ONE
	
	var resized := _resize_image(fetch_size)
	
	draw_size = Vector2i(view_size.floor()) + Vector2i.ONE
	draw_origin = Vector2i(view_origin.floor())
	position = draw_origin
	
	var img := Grid.get_cell_data(
		_image_size,
		Rect2i(draw_origin, draw_size)
	)
	
	if resized:
		_tex.set_image(img)
	else:
		_tex.update(img)

func _resize_image(new_size: Vector2i) -> bool:
	new_size = Vector2i(
		nearest_po2(new_size.x),
		nearest_po2(new_size.y)
	)
	
	if new_size != _image_size:
		var max_size := new_size * 2
		if new_size.x > _image_size.x || new_size.y > _image_size.y || _image_size.x > max_size.x || _image_size.y > max_size.y:
			_image_size = new_size
			print("new img size: ", _image_size)
			return  true
	
	return false

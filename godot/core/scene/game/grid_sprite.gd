extends Sprite2D
class_name  GridSprite

var _image_size := Vector2i.ZERO

@onready var _tex := ImageTexture.new()

func _ready() -> void:
	texture = _tex
	(material as ShaderMaterial).set_shader_parameter("materials_data", CellMaterials.cell_materials_data_texture)

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	var view_origin := -ctrans.get_origin() / ctrans.get_scale()
	var view_size := get_viewport_rect().size / ctrans.get_scale()
	
	var draw_size := Vector2i(view_size) + Vector2i.ONE
	var resized := _resize(draw_size)
	
	var draw_position := Vector2i(view_origin.floor())
	position = draw_position
	
	var img := Grid.get_cell_data(
		_image_size,
		Rect2i(draw_position, draw_size)
	)
	
	if resized:
		_tex.set_image(img)
	else:
		_tex.update(img)

func _resize(new_size: Vector2i) -> bool:
	new_size = Vector2i(
		nearest_po2(new_size.x),
		nearest_po2(new_size.y)
	)
	
	var max_size := new_size * 2
	if new_size != _image_size:
		if new_size.x > _image_size.x || new_size.y > _image_size.y || _image_size.x > max_size.x || _image_size.y > max_size.y:
			_image_size = new_size
			print("new img size: ", _image_size)
			return  true
	
	return false

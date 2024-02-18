extends Node2D
class_name GridRender

## Render cells from raw grid data.

static var node : GridRender

const MAX_GRID_CHUNK_SIZE := Vector2i(64, 64)

## Need cells data beyong what the screen can see,
## so that rendered light does not pop-in.
var cell_padding := Vector2(32.0, 32.0)

## What the camera sees.
var view := Rect2()
## Part of the grid which is rendered.
var raw_cell_rect := Rect2i()
var _last_raw_cell_size := Vector2i()

@onready var cell_render_material : ShaderMaterial = $Foreground.material

@onready var cell_raw_data_foreground : ImageTexture = $Foreground.texture
@onready var cell_raw_data_midground : ImageTexture = $Midground.texture
@onready var cell_raw_data_background : ImageTexture = $Backgroud.texture

func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	view = Rect2(
		-ctrans.get_origin() / ctrans.get_scale(),
		get_viewport_rect().size / ctrans.get_scale())
	
	var grid_chunk_start := Vector2i(((view.position - cell_padding) / 32.0).floor())
	var grid_chunk_end := Vector2i(((view.end + cell_padding) / 32.0).ceil())
	var grid_chunk_rect := Rect2i(grid_chunk_start, grid_chunk_end - grid_chunk_start)
	grid_chunk_rect.size = grid_chunk_rect.size.clamp(Vector2i.ZERO, MAX_GRID_CHUNK_SIZE)
	
	raw_cell_rect = Rect2i(grid_chunk_rect.position * 32, grid_chunk_rect.size * 32)
	# Avoid resizing cell texture for small size difference.
	if _last_raw_cell_size != raw_cell_rect.size:
		if  _last_raw_cell_size.x >= raw_cell_rect.size.x && _last_raw_cell_size.y >= raw_cell_rect.size.y:
			var dif := _last_raw_cell_size - raw_cell_rect.size
			if dif.x < 64 && dif.y < 64:
				raw_cell_rect.size = _last_raw_cell_size
				grid_chunk_rect.size = _last_raw_cell_size / 32
	_last_raw_cell_size = raw_cell_rect.size
	
	var fg_buffer := Grid.get_cell_buffer(grid_chunk_rect, Grid.GRID_LAYER_FOREGROUND)
	var mg_buffer := Grid.get_cell_buffer(grid_chunk_rect, Grid.GRID_LAYER_MIDGROUND)
	var bg_buffer := Grid.get_cell_buffer(grid_chunk_rect, Grid.GRID_LAYER_BACKGROUND)
	if raw_cell_rect.size != Vector2i(cell_raw_data_foreground.get_size()):
		cell_raw_data_foreground.set_image(fg_buffer)
		cell_raw_data_midground.set_image(mg_buffer)
		cell_raw_data_background.set_image(bg_buffer)
		#print("New raw cell texture size: ", raw_cell_rect.size)
	else:
		cell_raw_data_foreground.update(fg_buffer)
		cell_raw_data_midground.update(mg_buffer)
		cell_raw_data_background.update(bg_buffer)
	
	cell_render_material.set_shader_parameter(&"origin", raw_cell_rect.position)
	
	position = raw_cell_rect.position


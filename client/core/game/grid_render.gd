extends Node2D
class_name GridRender

## Render cells from raw grid data.

static var node : GridRender

## Cap cell buffer size to 2048x2048 
## to prevent lag/crash when experimenting.
const MAX_GRID_CHUNK_SIZE := Vector2i(64, 64)

## Need cells data beyond what the screen can see,
## so that lights do not pop-in.
var cell_padding := Vector2(50.0, 50.0)

## What the camera sees.
static var view := Rect2()
## Part of the grid which is rendered.
## Always mutiple of 32.
static var raw_cell_rect := Rect2i()
## Same as raw_cell_rect, but in chunks.
static var raw_cell_rect_chunk := Rect2i()
static var _last_raw_cell_size := Vector2i()

@export var background_light_modulate_viewport : SubViewport
@export var foreground_light_modulate_viewport : SubViewport
@export var light_pass_viewport : SubViewport

@onready var cell_render_material : ShaderMaterial = $Foreground.material
@export var cell_light_material : ShaderMaterial

@onready var cell_raw_data_foreground : ImageTexture = $Foreground.texture
@onready var cell_raw_data_background : ImageTexture = $Backgroud.texture

func _init() -> void:
	node = self

func _ready() -> void:
	light_pass_viewport.set_world_2d(get_world_2d())

func _process(_delta: float) -> void:
	var ctrans := get_canvas_transform()
	view = Rect2(
		-ctrans.get_origin() / ctrans.get_scale(),
		get_viewport_rect().size / ctrans.get_scale())
	
	var grid_chunk_start := Vector2i(((view.position - cell_padding) / 32.0).floor())
	var grid_chunk_end := Vector2i(((view.end + cell_padding) / 32.0).ceil())
	raw_cell_rect_chunk = Rect2i(grid_chunk_start, grid_chunk_end - grid_chunk_start)
	raw_cell_rect_chunk.size = raw_cell_rect_chunk.size.clamp(Vector2i.ZERO, MAX_GRID_CHUNK_SIZE)
	
	raw_cell_rect = Rect2i(raw_cell_rect_chunk.position * 32, raw_cell_rect_chunk.size * 32)
	# Avoid resizing raw cell texture for small size difference.
	if _last_raw_cell_size != raw_cell_rect.size:
		var dif := _last_raw_cell_size - raw_cell_rect.size
		if dif.x >= 0 && dif.x < 64:
			raw_cell_rect.size.x = _last_raw_cell_size.x
		if dif.y >= 0 && dif.y < 64:
			raw_cell_rect.size.y = _last_raw_cell_size.y
		raw_cell_rect_chunk.size = raw_cell_rect.size / 32
	_last_raw_cell_size = raw_cell_rect.size
	
	var fg_buffer := Grid.get_cell_buffer(raw_cell_rect_chunk, false)
	var bg_buffer := Grid.get_cell_buffer(raw_cell_rect_chunk, true)
	if raw_cell_rect.size != Vector2i(cell_raw_data_foreground.get_size()):
		cell_raw_data_foreground.set_image(fg_buffer)
		cell_raw_data_background.set_image(bg_buffer)
		print("New raw cell texture size: ", raw_cell_rect.size)
	else:
		cell_raw_data_foreground.update(fg_buffer)
		cell_raw_data_background.update(bg_buffer)
	
	position = raw_cell_rect.position
	
	RenderingServer.global_shader_parameter_set(&"cell_buffer_origin", raw_cell_rect.position)
	RenderingServer.global_shader_parameter_set(&"cell_buffer_size", Vector2(raw_cell_rect.size))
	
	background_light_modulate_viewport.size = raw_cell_rect.size
	foreground_light_modulate_viewport.size = raw_cell_rect.size
	light_pass_viewport.size = raw_cell_rect.size
	light_pass_viewport.canvas_transform.origin = -position


#func set_environment_light_color(col: Color) -> void:
	#environment_light.material.set_shader_parameter(&"light_color", col)

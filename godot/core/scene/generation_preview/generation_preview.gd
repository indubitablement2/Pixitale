extends Node2D

const IMG_SIZE = 1024

@export var world_size := Vector2i(32768, 16384)
@export var show_border := Vector2i(256, 256)

var _i := 0
var _sps : Array[Sprite2D] = []
var _update_all := 999999

func _ready() -> void:
	GridRender.set_enabled(false)
	
	randomize()
	WorldGeneration.generation_started.connect(_on_generation_started)
	WorldGeneration.generation_finished.connect(_on_generation_finished, 1)
	
	WorldGeneration.call_deferred("generate_world", world_size.x, world_size.y, randi())

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("up"):
		WorldGeneration.generate_world(world_size.x, world_size.y, randi())

func _process(_delta: float) -> void:
	if WorldGeneration.is_generating() && !_sps.is_empty():
		_i = (_i + 1) % _sps.size()
		var sp := _sps[_i]
		_update_sp(sp)
	elif _update_all < _sps.size():
		var sp := _sps[_update_all]
		_update_all += 1
		_update_sp(sp)

func _draw() -> void:
	draw_rect(Rect2(-show_border, Grid.get_size() + show_border * 2), Color.BLANCHED_ALMOND, false)

func _update_sp(sp: Sprite2D) -> void:
	var img := Grid.get_cell_data(Vector2i(IMG_SIZE, IMG_SIZE), Rect2i(sp.position, Vector2i(IMG_SIZE, IMG_SIZE)))
	sp.texture.update(img)

func _on_generation_started() -> void:
	queue_redraw()
	
	_i = 0
	
	for sp in _sps:
		sp.queue_free()
	_sps.clear()
	
	var pos := -show_border
	var pos_end := Grid.get_size() + show_border
	
	while pos.y < pos_end.y:
		pos.x = -show_border.x
		
		while pos.x < pos_end.x:
			var sp := Sprite2D.new()
			sp.centered = false
			sp.position = pos
			sp.texture = ImageTexture.create_from_image(Image.create(IMG_SIZE, IMG_SIZE, false, Image.FORMAT_RF))
			sp.material = preload("res://core/shader/cell.material")
			add_child(sp)
			_sps.push_back(sp)
			
			pos.x += IMG_SIZE
		
		pos.y += IMG_SIZE

func _on_generation_finished(_canceled: bool) -> void:
	_update_all = 0

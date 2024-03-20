extends Object
class_name Core

## Called once after mod is added.
static func _entry() -> void:
	_QUEUE_STEP_CHUNKS = GridApi.add_grid_edit_method(Callable(Core, &"_queue_step_chunks"))
	_SET_PAUSED = GridApi.add_grid_edit_method(Callable(Core, &"_set_paused"))
	_SET_CELL_MATERIAL_RECT = GridApi.add_grid_edit_method(Callable(Core, &"_set_cell_material_rect"))
	_SET_COLOR_RECT = GridApi.add_grid_edit_method(Callable(Core, &"_set_color_rect"))
	_SET_CELL_MATERIAL_FILL = GridApi.add_grid_edit_method(Callable(Core, &"_set_cell_material_fill"))

## Called before mod is removed.
## Any change made by _entry that could be permanent should be undone here.
static func _exit() -> void:
	pass

# Static methods which can safely modify Grid.
# Methods are networked and called on remote peers.
# This also help ensure determinism
# (as long as methods are deterministic themself).
# Implementation:
static func _queue_step_chunks(chunk_rect: Rect2i) -> void:
	Grid.queue_step_chunks(chunk_rect)
# Boilerplate:
static var _QUEUE_STEP_CHUNKS := 0
static func queue_step_chunks(chunk_rect: Rect2i) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([chunk_rect, _QUEUE_STEP_CHUNKS])

static func _set_paused(paused: bool) -> void:
	Game._set_paused(paused)
static var _SET_PAUSED := 0
## See Game.is_paused()
static func set_paused(paused: bool) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([paused, _SET_PAUSED])

static func _set_cell_material_rect(cell_material_idx: int, rect: Rect2i) -> void:
	var iter := Grid.iter_rect(rect)
	iter.fill_remaining(cell_material_idx)
static var _SET_CELL_MATERIAL_RECT := 0
static func set_cell_material_rect(cell_material_idx: int, rect: Rect2i) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([cell_material_idx, rect, _SET_CELL_MATERIAL_RECT])

static func _set_color_rect(color: int, rect: Rect2i) -> void:
	var iter := Grid.iter_rect(rect)
	while iter.next():
		iter.set_color(color)
static var _SET_COLOR_RECT := 0
static func set_color_rect(color: int, rect: Rect2i) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([color, rect, _SET_COLOR_RECT])

static func _set_cell_material_fill(cell_material_idx: int, start: Vector2i) -> void:
	var filter := Grid.get_cell_material_idx(start)
	var iter := Grid.iter_fill(start, filter)
	iter.fill_remaining(cell_material_idx)
	#while iter.next():
		#print(iter.coord())
		#iter.set_material_idx(cell_material_idx)
static var _SET_CELL_MATERIAL_FILL := 0
static func set_cell_material_fill(cell_material_idx: int, start: Vector2i) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([cell_material_idx, start, _SET_CELL_MATERIAL_FILL])


extends Object
class_name Core

## Called once after mod is added.
static func _entry() -> void:
	_QUEUE_STEP_CHUNKS = GridApi.add_grid_edit_method(Callable(Core, &"_queue_step_chunks"))
	_SET_PAUSED = GridApi.add_grid_edit_method(Callable(Core, &"_set_paused"))
	
	print("core added")

## Called before mod is removed.
## Any change made by _entry that could be permanent should be undone here.
static func _exit() -> void:
	print("core removed")

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

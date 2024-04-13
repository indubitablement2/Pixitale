extends Node
class_name Game

## Game root.

static var node : Game

static func is_paused() -> bool:
	return node.process_mode == Node.PROCESS_MODE_DISABLED

static func _set_paused(paused: bool) -> void:
	var was_paused := is_paused()
	
	if paused:
		node.process_mode = Node.PROCESS_MODE_DISABLED
	else:
		node.process_mode = Node.PROCESS_MODE_INHERIT
	
	if was_paused != is_paused():
		node.is_paused_changed.emit()

signal is_paused_changed


@export var root_items : Node

# TODO: Save/share
## Not the same as Grid.get_tick()
## which may increment even when game is paused.
## This better represent time.
## Deterministic.
static var tick := 0

func _init() -> void:
	node = self

func _process(_delta: float) -> void:
	tick += 1
	
	var step_start := Vector2i(((GridRender.view.position - Vector2(64.0, 64.0)) / 32.0).floor())
	var step_end := Vector2i(((GridRender.view.end + Vector2(64.0, 64.0)) / 32.0).ceil())
	Core.queue_step_chunks(Rect2i(step_start, step_end - step_start))

@rpc("authority", "call_local", "reliable")
static func spawn_item(item_data_path: String, at: Vector2, qty: int) -> void:
	var item : ItemInstance = load(item_data_path).make_instance(qty)
	var ist : ItemWorld = preload("res://core/game/item.tscn").instantiate()
	ist.position = at
	ist.set_item(item)
	node.root_items.add_child(ist)

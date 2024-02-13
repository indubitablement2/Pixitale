extends Object
class_name CoreEdits

static var _QUEUE_STEP_CHUNKS := 0
static func queue_step_chunks(chunk_rect: Rect2i) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([chunk_rect, _QUEUE_STEP_CHUNKS])

static func _queue_step_chunks(chunk_rect: Rect2i) -> void:
	Grid.queue_step_chunks(chunk_rect)

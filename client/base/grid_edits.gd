extends Object
class_name Banana

static var _BANANA := 0
static func banana(num: int) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([num, _BANANA])

static func _banana(num: int) -> void:
	var iter := Grid.iter_chunk(Vector2i())
	iter.fill_remaining(num)

static var _APPLE := 0
static func apple(is_true: bool, other) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([is_true, other, _APPLE])

static func _apple(is_true: bool, other) -> void:
	print(is_true, other)

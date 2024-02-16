extends Node
class_name Base

static var ROCK : CellMaterial
static var ROCK_IDX := 0

static func _entry() -> void:
	ROCK = GridApi.find_cell_material(&"Rock")
	ROCK_IDX = ROCK.idx
	
	_BANANA = GridApi.add_grid_edit_method(Callable(Base, &"_banana"))
	
	print("base added")

static func _exit() -> void:
	print("base removed")

static func _banana(num: int) -> void:
	print("banana = ", num, " | ", _BANANA)
static var _BANANA := 0
static func banana(num: int) -> void:
	if GridApi.is_server:
		GridApi._next_edits.push_back([num, _BANANA])

extends Object
class_name Base

static var ROCK : CellMaterial
static var ROCK_IDX := 0
static var WATER : CellMaterial
static var WATER_IDX := 0
static var LAVA : CellMaterial
static var LAVA_IDX := 0
static var SAND : CellMaterial
static var SAND_IDX := 0

static func _entry() -> void:
	ROCK = GridApi.find_cell_material(&"Rock")
	ROCK_IDX = ROCK.idx
	WATER = GridApi.find_cell_material(&"Water")
	WATER_IDX = WATER.idx
	LAVA = GridApi.find_cell_material(&"Lava")
	LAVA_IDX = LAVA.idx
	SAND = GridApi.find_cell_material(&"Sand")
	SAND_IDX = SAND.idx
	
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

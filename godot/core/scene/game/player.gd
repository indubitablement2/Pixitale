extends GridCharacterBody
class_name Player

const ACCELERATION = Vector2(8.0, 0.0)
const GRAVITY = 12.0

@onready var m := CellMaterials.get_cell_materials_idx("")
@onready var m1 := CellMaterials.get_cell_materials_idx("water")
@onready var m2 := CellMaterials.get_cell_materials_idx("sand")
@onready var m3 := CellMaterials.get_cell_materials_idx("test")
@onready var m4 := CellMaterials.get_cell_materials_idx("lava")
@onready var m5 := CellMaterials.get_cell_materials_idx("rock")

var brush_size := 17

func _ready() -> void:
	GameGlobals.player = self

func _exit_tree() -> void:
	if GameGlobals.player == self:
		GameGlobals.player = null

func _process(delta: float) -> void:
	var dir := Vector2(
		Input.get_action_strength("right") - Input.get_action_strength("left"),
		Input.get_action_strength("down") - Input.get_action_strength("up")
	)
	
	if Input.is_action_just_pressed("up"):
		velocity.y -= 5.0
	
	velocity += dir * ACCELERATION * delta
	velocity.y += GRAVITY * delta
	velocity *= 0.95
	move()
	
	GameGlobals.player_position = position
	
	if Input.is_action_pressed("attack"):
		_set_rect(m)
	elif Input.is_action_pressed("use_item_1"):
		_set_rect(m1)
	elif Input.is_action_pressed("use_item_2"):
		_set_rect(m2)
	elif Input.is_action_pressed("use_item_3"):
		_set_rect(m3)
	elif Input.is_action_pressed("use_item_4"):
		_set_rect(m4)
	elif Input.is_action_pressed("use_item_5"):
		_set_rect(m5)

func _set_rect(cell_material_idx : int) -> void:
	var set_cell_position := Vector2i(GameGlobals.mouse_position)
	
	Grid.set_cell_rect(Rect2i(
		set_cell_position - (Vector2i(brush_size, brush_size) - Vector2i.ONE) / 2,
		Vector2i(brush_size, brush_size)
		), cell_material_idx)

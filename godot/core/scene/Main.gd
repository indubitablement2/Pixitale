extends Control

const NUM_CHUNK = 4
const GRID_SIZE = 64 + 32 * NUM_CHUNK

@onready var tex :ImageTexture
@onready var sp := Sprite2D.new()

func _ready() -> void:
	Grid.run_tests()
	
	var img := Image.create(GRID_SIZE + 64, GRID_SIZE, false, Image.FORMAT_RGBA8)
	tex = ImageTexture.create_from_image(img)
	
	add_child(sp)
	sp.centered = false
	sp.scale = Vector2(Grid.GRID_SCALE, Grid.GRID_SCALE)
	sp.set_texture(tex)
	
	var mat := ShaderMaterial.new()
	mat.set_shader(preload("res://core/shader/cell.gdshader"))
	sp.set_material(mat)
	
	Grid.new_empty(GRID_SIZE + 64, GRID_SIZE)
	Grid.update_texture_data(tex, Vector2i(0, 0))
	
	print("grid size: ", Grid.get_size())
	print("grid size chunk: ", Grid.get_size_chunk())

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("down"):
		Grid.step_manual()
		Grid.update_texture_data(tex, Vector2i(0, 0))

func _process(_delta: float) -> void:
	if Input.is_action_pressed("up"):
		Grid.step_manual()
		Grid.update_texture_data(tex, Vector2i(0, 0))
	
	var mouse_pos := get_global_mouse_position() / Grid.GRID_SCALE
	var grid_pos := Vector2i(mouse_pos)
	var mat_idx := Grid.get_cell_material_idx(grid_pos)
	
	$CellName.set_text(CellMaterials.cell_material_names[mat_idx])
	$Tick.set_text(str(Grid.get_tick()))
	$ChunkActive.set_text(str(Grid.is_chunk_active(Vector2i(1, 1))))
	$GridPosition.set_text(str(grid_pos))


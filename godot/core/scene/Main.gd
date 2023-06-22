extends Control

var t := 0

@export var wish_size := GameGlobals.WORLD_SIZE.TINY

func _ready() -> void:
	WorldGeneration.generation_finished.connect(_on_generation_finished, 1)
	
	randomize()
	WorldGeneration.call_deferred("generate_world", wish_size, randi())

func _on_generation_finished(_canceled: bool) -> void:
	Background.start()
	process_mode = Node.PROCESS_MODE_INHERIT

func _process(_delta: float) -> void:
	Grid.step_manual()
	
	var mouse_pos := get_global_mouse_position()
	var grid_pos := Vector2i(mouse_pos)
	var chunk_pos := grid_pos / Vector2i(32, 32)
	var mat_idx := Grid.get_cell_material_idx(grid_pos)

	$CellName.set_text(CellMaterials.cell_materials[mat_idx].display_name)
	$Tick.set_text(str(Grid.get_tick()))
	$ChunkActive.set_text(str(Grid.is_chunk_active(chunk_pos)))
	$GridPosition.set_text(str(grid_pos))


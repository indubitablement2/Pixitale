extends Control

@onready var _label_cell_material_name : Label = $CellMaterialName

func _process(_delta: float) -> void:
	var mouse_pos_i := Vector2i(Game.mouse_position)
	
	_label_cell_material_name.set_text(
		CellMaterials
			.cell_materials[Grid.get_cell_material_idx(mouse_pos_i)]
			.display_name
	)

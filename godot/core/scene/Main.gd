extends Control

@onready var tex := ImageTexture.new()
@onready var sp := Sprite2D.new()

func _ready() -> void:
	add_child(sp)
	sp.centered = false
	sp.scale = Vector2(Grid.GRID_SCALE, Grid.GRID_SCALE)
	sp.set_texture(tex)
	var mat := ShaderMaterial.new()
	mat.set_shader(preload("res://core/shader/cell.gdshader"))
	sp.set_material(mat)
	
	Grid.new_empty(512, 512)
	print(Grid.get_size())

func _process(_delta: float) -> void:
	Grid.step_manual()
	Grid.set_texture_data(tex, Rect2i(0.0, 0.0, 512, 512))
	

#func _draw() -> void:
#	Grid.set_texture_data(tex, Rect2i(0.0, 0.0, 64, 64))
#	draw_texture(tex, Vector2.ZERO)

extends Control

@onready var tex := ImageTexture.new()
@onready var sp := Sprite2D.new()

func _ready() -> void:
	add_child(sp)
	sp.centered = false
	sp.scale = Vector2(Grid.CELL_SIZE, Grid.CELL_SIZE)
	sp.set_texture(tex)
	
	Grid.new_empty(512, 512)
	print(Grid.get_size())

func _process(_delta: float) -> void:
	Grid.set_texture_data(tex, Rect2i(0.0, 0.0, 512, 512))
	

#func _draw() -> void:
#	Grid.set_texture_data(tex, Rect2i(0.0, 0.0, 64, 64))
#	draw_texture(tex, Vector2.ZERO)

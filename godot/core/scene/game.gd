extends Node2D

@onready var camera := $Camera
@onready var grid_sprite : GridSprite = $GridSprite

func _ready() -> void:
#	Grid.run_tests()
	
	Grid.new_empty(256, 256)
	
#	grid_sprite.draw_size = Vector2i(1920 / 4, 1080 / 4)

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, Grid.get_size()), Color.CHARTREUSE, false, 1.0)

func _process(_delta: float) -> void:
	Grid.step_manual()
	
#	grid_sprite.draw_position = get_global_mouse_position()
	

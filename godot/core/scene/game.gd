extends Node2D

var grid_size := Vector2i(256, 256)

@onready var camera := $Camera
@onready var grid_sprite : GridSprite = $GridSprite

func _ready() -> void:
#	Grid.run_tests()
	
	Grid.new_empty(grid_size.x, grid_size.y)
	grid_size = Grid.get_size()
	
	grid_sprite.draw_size = Vector2i(1920 / 4, 1080 / 4)

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, grid_size), Color.CHARTREUSE, false, 1.0)

func _process(_delta: float) -> void:
	Grid.step_manual()
	
	grid_sprite.draw_position = get_global_mouse_position()
	

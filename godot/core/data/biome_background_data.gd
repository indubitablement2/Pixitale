extends Resource
class_name PanoramaLayerData

const TILING_WIDTH = 2000000

@export var texture : Texture2D
@export var movement_scale := Vector2.ONE

func to_sprite() -> Sprite2D:
	var sp := Sprite2D.new()
	sp.centered = false
	sp.region_enabled = true
	
	sp.region_rect = Rect2(0, 0, TILING_WIDTH, texture.get_height())
	
	sp.texture = texture
	
	return sp

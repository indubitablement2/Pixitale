extends ParallaxBackground
class_name BiomeBackground

@export var fade : Array[NodePath] = []
@onready var _fade = fade.map(get_node)

var a := 1.0

func fade_out(a_delta: float) -> bool:
	a -= a_delta
	
	for n in _fade:
		n.modulate.a -= a_delta
	
	if a <= 0.0:
		queue_free()
		return true
	else:
		return false

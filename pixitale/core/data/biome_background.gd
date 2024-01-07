extends ParallaxBackground
class_name BiomeBackground

@export var fade : Array[NodePath] = []
@onready var _fade = fade.map(get_node)

func set_alpha(a: float) -> void:
	for n in _fade:
		n.modulate.a = a

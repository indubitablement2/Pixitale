extends Node
class_name VisibleEnabler

## Enable/disable parent CanvasItem based on its visibility.

@onready var parent : CanvasItem = get_parent()

func _ready() -> void:
	parent.visibility_changed.connect(_on_parent_visibility_changed)
	_on_parent_visibility_changed()

func _on_parent_visibility_changed() -> void:
	parent.set_process(parent.visible)
	parent.set_process_input(parent.visible)

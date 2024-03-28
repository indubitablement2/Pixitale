extends BaseButton

## Toggle target visibility when button is toggled.

@export var target : CanvasItem

func _ready() -> void:
	toggle_mode = true
	target.visibility_changed.connect(_on_target_visibility_changed)
	_on_target_visibility_changed()

func _toggled(toggled_on: bool) -> void:
	target.visible = toggled_on

func _on_target_visibility_changed() -> void:
	set_pressed_no_signal(target.visible)

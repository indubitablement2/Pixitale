extends Resource
class_name GemData


enum GemType {
	## Cast by the tower or by a proc.
	SPELL,
	## Modify the previous spell.
	SUPPORT,
	## Cast the next spell
	## when the previous spell meet a requirement.
	PROC
}


@export var display_name := ""

@export var gem_type := GemType.SPELL
## If this is a support,
## added as child to Spell when parsing gems data.
## Otherwise, this is the base Spell scene.
@export var scene : PackedScene = null
## Call support(spell: Spell) -> void
## when parsing gems data.
@export var support_script : GDScript = null


func apply_support(to: Spell) -> void:
	if scene:
		to.add_child(scene.instantiate())
	if support_script:
		support_script.new().support(to)


func _to_string() -> String:
	return display_name

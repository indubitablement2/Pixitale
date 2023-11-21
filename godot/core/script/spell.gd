extends Node2D
class_name Spell


@export var direct_cast_cooldown := 0.0


@export var spread := 0.0
@export var aoe := 32.0
@export var damage := 10.0

@export var num_cast := 1


signal enemy_killed(enemy: Enemy)
signal enemy_hit(enemy: Enemy)


func cast(from: Vector2, to: Vector2) -> void:
	for i in num_cast:
		_cast_duplicate(from, to)


func _cast_duplicate(from: Vector2, to: Vector2) -> void:
	var spell : Spell = duplicate()
	spell._prepare(from, to)
	Game.add_child(spell)


@warning_ignore("unused_parameter")
func _prepare(from: Vector2, to: Vector2) -> void:
	position = to + Vector2(
		randf_range(-spread, spread),
		randf_range(-spread, spread))

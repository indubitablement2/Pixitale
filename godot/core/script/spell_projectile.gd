extends Spell
class_name SpellProjectile


const MAX_PROJS_OFFSET := 0.2


@export var speed := Vector2(500.0, 0.0)
@export var pierce := 1
@export var radius := 32.0
@export var time_to_live := 0.8

@export var num_projectile := 1


var seen := {}


func _physics_process(delta: float) -> void:
	position += speed * delta
	
	for enemy in Grid.query(position, radius):
		if !seen.has(enemy.get_instance_id()):
			seen[enemy.get_instance_id()] = null
			
			if enemy.hit(damage):
				enemy_killed.emit(enemy)
			enemy_hit.emit(enemy)
			
			pierce -= 1
			if pierce <= 0:
				queue_free()
	
	time_to_live -= delta
	if time_to_live < 0.0:
		queue_free()


func _prepare(from: Vector2, to: Vector2) -> void:
	position = from
	
	look_at(to)
	rotate(randfn(-spread, spread))
	
	if num_projectile > 1:
		var proj_offset := minf(
			TAU / float(num_projectile), MAX_PROJS_OFFSET)
		rotate(proj_offset * float(num_projectile) * -0.5)
		speed = speed.rotated(rotation)
		
		for i in num_projectile - 1:
			Game.add_child(duplicate())
			
			rotate(proj_offset)
			speed = speed.rotated(proj_offset)
	else:
		speed = speed.rotated(rotation)

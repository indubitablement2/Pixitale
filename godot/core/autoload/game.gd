extends Node


@export var path : Path2D
@export var grid_dims := Vector3(1024.0, 1024.0, 64.0)


@export var turret_position := Vector2(300.0, 200.0)

@export var extra_cast_cooldown := 0.0
@export var global_support_gems : Array[GemData] = []

@export var gems: Array[GemData] = []


var next_gem_idx := 0
var parsed_gems : Array[Spell] = []
var current_cast_cooldown := 0.0


func _ready() -> void:
	parse_gems()
	
	for _i in 10:
		spawn_enemy()
	
	await get_tree().create_timer(0.1).timeout
	for c in path.get_children():
		c.progress_ratio = randf()


func _physics_process(delta: float) -> void:
	Grid.new_empty(grid_dims.x, grid_dims.y, grid_dims.z)
	
#	spawn_enemy()
#	print(path.get_child_count())
	
	current_cast_cooldown -= delta
	if path.get_child_count() > 0 && current_cast_cooldown < 0.0 && !parsed_gems.is_empty():
		cast(path.get_child(0))


func spawn_enemy() -> void:
	var enemy := preload("res://base/enemy/blue.tscn").instantiate()
	enemy.h_offset = randf_range(-32.0, 32.0)
	enemy.v_offset = randf_range(-32.0, 32.0)
	path.add_child(enemy)


func cast(target: Enemy) -> void:
	parsed_gems[next_gem_idx].cast(turret_position, target.position)
	current_cast_cooldown = parsed_gems[next_gem_idx].direct_cast_cooldown
	
	next_gem_idx += 1
	if next_gem_idx >= parsed_gems.size():
		current_cast_cooldown += extra_cast_cooldown
		next_gem_idx = 0


func parse_gems() -> void:
	parsed_gems = []
	
	# Find first spell gem idx.
	var first_spell_gem_idx := 0
	while true:
		if first_spell_gem_idx >= gems.size():
			# No spell gem found.
			return
		if gems[first_spell_gem_idx].gem_type == GemData.GemType.SPELL:
			break
		first_spell_gem_idx += 1
	
	# Splits gems into groups.
	# Spell followed by supports
	var groups : Array[Array] = []
	var gem_idx := first_spell_gem_idx
	while true:
		var gem := gems[gem_idx]
		
		if gem.gem_type == GemData.GemType.SPELL:
			var group : Array[GemData] = []
			group.push_back(gem)
			group.append_array(global_support_gems)
			groups.push_back(group)
		else:
			groups[-1].push_back(gem)
		
		gem_idx = (gem_idx + 1)  % gems.size()
		if gem_idx == first_spell_gem_idx:
			break
	
	print(groups)
	
	# Parse groups.
	while !groups.is_empty():
		parsed_gems.push_back(_parse_group(groups.pop_front(), groups))

func _parse_group(group: Array[GemData], groups: Array) -> Spell:
	var spell : Spell = group.pop_front().scene.instantiate()
	
	while !group.is_empty():
		var support : GemData = group.pop_front()
		
		if support.gem_type == GemData.GemType.SUPPORT:
			support.apply_support(spell)
		elif support.gem_type == GemData.GemType.PROC:
			# TODO: Proc gems
			#if !groups.is_empty():
				#var proc_spell := _parse_group(groups.pop_front(), groups)
			push_error("todo proc gem")
	
	return spell


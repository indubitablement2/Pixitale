extends Node

@onready var _thread := Thread.new()
var _is_canceled := false
var _passes : Array[GenerationPass] = []

signal generation_started
signal generation_pass_changed(current_pass: GenerationPass)
signal generation_finished(canceled: bool)

func is_generating() -> bool:
	return _thread.is_started()

func generate_world(size: Game.WORLD_SIZE, base_seed: int) -> void:
	if is_generating():
		push_error("is already generating world")
		return
	
	Game.world_size = size
	var s := Game.get_wish_world_size()
	Grid.new_empty(s.x, s.y)
	Grid.set_seed(base_seed)
	seed(base_seed)
	
	Game.compute_layers_starts()
	Game.background_offset.x = randf() * 1024.0
	Game.update_background_y_offset()
	
	_thread.start(_generate)
	
	generation_started.emit()

func cancel_generation() -> void:
	if is_generating():
		_is_canceled = true

func get_pass(pass_id: String) -> GenerationPass:
	for p in _passes:
		if p.id == pass_id:
			return p
	
	push_error(pass_id, "does not exist")
	return null

func get_passes() -> Array[GenerationPass]:
	if is_generating():
		push_error("can not modify passes while generating")
		return []
	return _passes

func _generation_pass_changed(current_pass: GenerationPass) -> void:
	generation_pass_changed.emit(current_pass)

func _generation_finished() -> void:
	_thread.wait_to_finish()
	generation_finished.emit(_is_canceled)
	_is_canceled = false

func _generate() -> void:
	var start_start := Time.get_ticks_msec()
	var start := start_start
	var end := start
	
	for p in _passes:
		if _is_canceled:
			break
		
		if p.skip:
			print(p.id, " skipped.")
			continue
		
		call_deferred("_generation_pass_changed", p)
		
		p.generate()
		
		end = Time.get_ticks_msec()
		print(p.id , " in ", end - start, "ms.")
		start = end
	
	print("Generation done in ", end - start_start, "ms.")
	
	call_deferred("_generation_finished")

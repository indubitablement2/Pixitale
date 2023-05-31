extends Node

@onready var _thread := Thread.new()
var _is_canceled := false
var _passes : Array[Resource] = []

signal generation_started
signal generation_pass_changed(pass_name: String)
signal generation_finished(canceled: bool)

func is_generating() -> bool:
	return _thread.is_started()

func generate_world(wish_width: int, wish_height: int, base_seed: int) -> void:
	if is_generating():
		push_error("is already generating world")
		return
	
	Grid.new_empty(wish_width, wish_height)
	Grid.set_seed(base_seed)
	seed(base_seed)
	
	_thread.start(_generate)
	
	generation_started.emit()

func cancel_generation() -> void:
	if is_generating():
		_is_canceled = true

# Each passes should have at lease these 2 functions:
# `func generate() -> void`
# `func pass_name -> String`
func get_passes() -> Array[Resource]:
	if is_generating():
		push_error("can not modify passes while generating")
		return []
	return _passes

func get_pass(pass_name: String) -> Resource:
	for p in _passes:
		if p.pass_name() == pass_name:
			return p
	push_error(pass_name, " does not exist")
	return null

func _generation_pass_changed(pass_name: String) -> void:
	generation_pass_changed.emit(pass_name)

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
		
		call_deferred("_generation_pass_changed", p.pass_name())
		
		p.generate()
		
		end = Time.get_ticks_msec()
		print(p.pass_name() , " in ", end - start, "ms.")
		start = end
	
	call_deferred("_generation_pass_changed", "Finishing touches")
	Grid.post_generation_pass()
	end = Time.get_ticks_msec()
	print("Post generation in ", end - start, "ms.")
	
	start = end
	call_deferred("_generation_pass_changed", "Settling things down")
	# TODO: Step simulation ~4k times.
	end = Time.get_ticks_msec()
	print("Simulation in ", end - start, "ms.")
	
	print("Generation done in ", end - start_start, "ms.")
	
	call_deferred("_generation_finished")

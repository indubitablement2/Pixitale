extends Node

@onready var _thread := Thread.new()
var _is_canceled := false
var _passes : Array[WorldGenerationPass] = []

signal generation_started
signal generation_pass_changed(pass_name: String)
signal generation_finished(canceled: bool)

func is_generating() -> bool:
	return _thread.is_started()

func generate_world(wish_width: int, wish_height: int, base_seed: int) -> void:
	if is_generating():
		push_error("is already generating world")
		return
	
	generation_started.emit()
	
	Grid.new_empty(wish_width, wish_height)
	Grid.set_seed(base_seed)
	seed(base_seed)
	
	_thread.start(_generate)

func cancel_generation() -> void:
	if is_generating():
		_is_canceled = true

func get_passes() -> Array[WorldGenerationPass]:
	if is_generating():
		push_error("can not modify passes while generating")
		return []
	return _passes

func _generation_pass_changed(pass_name: String) -> void:
	generation_pass_changed.emit(pass_name)

func _generation_finished() -> void:
	_thread.wait_to_finish()
	generation_finished.emit(_is_canceled)
	_is_canceled = false

func _generate() -> void:
	for p in _passes:
		if _is_canceled:
			break
		
		call_deferred("_generation_pass_changed", p.pass_name)
		p.generate()
	
	call_deferred("_generation_finished")

extends GenerationPass

@export var num_simulation_step := 100

func generate() -> void:
	var i := 0
	while i < num_simulation_step:
		Grid.step_manual()
		i += 1

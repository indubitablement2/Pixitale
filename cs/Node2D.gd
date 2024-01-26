extends Node2D


@export var noise : FastNoiseLite


func _process(delta: float) -> void:
	var sum := 0.0
	for i in 1:
		var t := Time.get_ticks_msec()
		for y in 1000:
			for x in 1000:
				sum += noise.get_noise_2d(float(x), float(y))
		print(Time.get_ticks_msec() - t)
	print(sum)

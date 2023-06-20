extends BiomeBackground

func fade_out(a_delta: float) -> bool:
	$space_bg.modulate.a -= a_delta
	
	if $space_bg.modulate.a <= 0.0:
		queue_free()
		return true
	else:
		return false

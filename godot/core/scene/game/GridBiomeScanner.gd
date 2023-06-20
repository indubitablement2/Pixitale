extends GridBiomeScanner

const BG_TRANSITION_SPEED := 1.0
var biome_bgs : Array[BiomeBackground] = []

var current_biome : BiomeData

var current_biome_bg : BiomeBackground = null
var new_biome_bg : BiomeBackground = null
var transition := false

func _process(delta: float) -> void:
	position = GameGlobals.player_position
	
	if transition:
		if current_biome_bg == null:
			transition = false
			current_biome_bg = new_biome_bg
			new_biome_bg = null
		elif current_biome_bg.fade_out(delta * BG_TRANSITION_SPEED):
			current_biome_bg = null
	elif scan():
		transition = true
		current_biome = Mod.biomes_data[get_current_biome()]
		
		print("Biome changed: ", current_biome.id)
		
		if current_biome.background != null:
			new_biome_bg = current_biome.background.instantiate()
			new_biome_bg.layer = -101
			add_child(new_biome_bg)
		
		if current_biome_bg:
			current_biome_bg.layer = -100


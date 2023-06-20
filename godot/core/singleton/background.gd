extends Node2D

const BG_TRANSITION_SPEED := 1.0

var current_biome : BiomeData = null
var current_biome_bg : BiomeBackground = null

var new_biome : BiomeData = null
var new_biome_bg : BiomeBackground = null

@onready var scanner : GridBiomeScanner = $GridBiomeScanner

func _process(delta: float) -> void:
	if new_biome:
		if current_biome_bg.fade_out(delta * BG_TRANSITION_SPEED):
			current_biome = new_biome
			new_biome = null
			
			current_biome_bg = new_biome_bg
			current_biome_bg.layer = -100
			new_biome_bg = null
			
			print("Biome changed: ", current_biome.id)
	else:
		scanner.position = GameGlobals.player_position
		if scanner.scan():
			if current_biome:
				new_biome = Mod.biomes_data[scanner.get_current_biome()]
				new_biome_bg = _make_bg(new_biome)
			else:
				current_biome = Mod.biomes_data[scanner.get_current_biome()]
				current_biome_bg = _make_bg(current_biome)
				current_biome_bg.layer = -100

func start() -> void:
	process_mode = Node.PROCESS_MODE_INHERIT
	$CanvasLayer/Node2D.position.y = GameGlobals.layer_cavern_start

func stop() -> void:
	current_biome = null
	if current_biome_bg:
		current_biome_bg.queue_free()
		current_biome_bg = null
	new_biome = null
	if new_biome_bg:
		new_biome_bg.queue_free()
		new_biome_bg = null
	
	process_mode = Node.PROCESS_MODE_DISABLED

func _make_bg(data: BiomeData) -> BiomeBackground:
	var bg : BiomeBackground
	if data.background:
		bg = data.background.instantiate()
	else:
		bg = BiomeBackground.new()
	
	bg.scroll_base_offset = GameGlobals.background_offset
	bg.layer = -101
	
	add_child(bg)
	
	return bg



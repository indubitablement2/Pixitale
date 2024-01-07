extends Node2D

const BG_TRANSITION_SPEED := 1.0

var current_biome : BiomeData = null
var current_biome_bg : BiomeBackground = null
@onready var current_biome_cavern : Sprite2D = $CanvasLayer/CurrentCavern

var new_biome : BiomeData = null
var new_biome_bg : BiomeBackground = null
@onready var new_biome_cavern : Sprite2D = $CanvasLayer/NewCavern

@onready var scanner : GridBiomeScanner = $GridBiomeScanner

func _process(delta: float) -> void:
	Game.mouse_position = get_global_mouse_position()
	
	if new_biome:
		var a_delta := delta * BG_TRANSITION_SPEED
		
		current_biome_cavern.modulate.a -= a_delta
		new_biome_cavern.modulate.a += a_delta
		
		current_biome_bg.set_alpha(current_biome_cavern.modulate.a)
#		new_biome_bg.set_alpha(new_biome_cavern.modulate.a)
		
		if current_biome_cavern.modulate.a <= 0.0:
			current_biome = new_biome
			new_biome = null
			
			current_biome_bg.queue_free()
			current_biome_bg = new_biome_bg
			new_biome_bg = null
			current_biome_bg.set_alpha(1.0)
			current_biome_bg.layer = -100
			
			current_biome_cavern.modulate.a = 1.0
			current_biome_cavern.texture = new_biome_cavern.texture
			new_biome_cavern.hide()
#			new_biome_cavern.modulate.a = 0.0
			
			print("Biome changed: ", current_biome.id)
	else:
		scanner.position = Game.player_position
		if scanner.scan():
			if current_biome:
				new_biome = Mod.biomes_data[scanner.get_current_biome()]
				
				new_biome_bg = _make_bg(new_biome)
#				new_biome_bg.set_alpha(0.0)
				
				new_biome_cavern.texture = new_biome.cavern_background
				new_biome_cavern.modulate.a = 0.0
				new_biome_cavern.show()
			else:
				current_biome = Mod.biomes_data[scanner.get_current_biome()]
				current_biome_bg = _make_bg(current_biome)
				current_biome_bg.layer = -100
				current_biome_cavern.texture = current_biome.cavern_background

func start() -> void:
	process_mode = Node.PROCESS_MODE_INHERIT
	$CanvasLayer.offset.y = Game.cavern_start_depth

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
	
	bg.scroll_base_offset = Game.background_offset
	bg.layer = -101
	
	add_child(bg)
	
	return bg

func _set_transition_texture(transition: Sprite2D, texture: Texture2D) -> void:
	transition.texture = texture
	transition.region_rect.size.y = float(texture.get_height())


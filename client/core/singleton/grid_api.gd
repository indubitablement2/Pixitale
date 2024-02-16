extends Node
## Gdscript friendly api over Grid.

# Editing Grid safely:
# While it is safe to read from Grid at any time, it is not to modify it.
# Instead use methods from ModEntry.grid_edits.

# Randomness and determinism:
# When generating chunk, use rand methods from GridChunkIter
# to ensure the same chunk is generated for all peers and
# regarless of Grid.tick.
#
# When doing other deterministic edits, use rand methods from Grid
# which change each step, but will be the same for all peers.

var is_server := true

var mod_names : Array[String] = ["core", "base"]

var mod_entries : Array[ModEntry] = []

# Automatically update texture when its image changes.
var base_color_atlas_img := Image.new()
var base_color_atlas := preload("res://core/shader/texture/cell_materials_data/base_color_atlas.tres")
var base_color_rect_img := Image.new()
var base_color_rect := preload("res://core/shader/texture/cell_materials_data/base_color_atlas_rect.tres")
var glow_img := Image.new()
var glow := preload("res://core/shader/texture/cell_materials_data/glow.tres")
var light_modulate_img := Image.new()
var light_modulate := preload("res://core/shader/texture/cell_materials_data/light_modulate.tres")
var _img_changed : Array[Array] = []

var cell_materials : Array[CellMaterial] = []
## StringName : CellMaterial
var cell_material_names := {}
## StringName : Array[CellMaterial]
var cell_material_tags := {}

## Ordered in this order
## - process_priority
## - which mod they are part of
## - their original node order
var generation_passes : Array[GenerationPass] = []

var _edit_callables : Array[Callable] = []
## tick(int) : Array of Array(args..., callback idx)
var _queued_edits := {}
## [args..., callable_idx(int)]
var _next_edits := []

var _step_thread := Thread.new()

func _ready() -> void:
	#process_mode = Node.PROCESS_MODE_DISABLED
	
	_img_changed = [
		[false, base_color_atlas_img, base_color_atlas],
		[false, base_color_rect_img, base_color_rect],
		[false, glow_img, glow],
		[false, light_modulate_img, light_modulate]
	]
	for img_idx in _img_changed.size():
		_img_changed[img_idx][1].changed.connect(_on_img_changed.bind(img_idx))

func _exit_tree() -> void:
	if _step_thread.is_started():
		_step_thread.wait_to_finish()
	#unload_mods()

func _process(_delta: float) -> void:
	for img_arr in _img_changed:
		if img_arr[0]:
			img_arr[0] = false
			var img := img_arr[1] as Image
			var tex := img_arr[2] as ImageTexture
			if Vector2i(tex.get_size()) != img.get_size():
				tex.set_image(img)
			else:
				tex.update(img)
	
	if is_server:
		# Send queued grid edits to peers
		if multiplayer.has_multiplayer_peer():
			_edit_peer(Grid.get_tick(), var_to_bytes(_next_edits))
		
		_queued_edits[Grid.get_tick()] = _next_edits
		_next_edits = []
	
	# Step up to 2 times if behind
	for i in 2:
		if _queued_edits.has(Grid.get_tick()):
			var edits := _queued_edits[Grid.get_tick()] as Array
			_queued_edits.erase(Grid.get_tick())
			
			# It safe to modify the grid when not stepping.
			if _step_thread.is_started():
				_step_thread.wait_to_finish()
			
			for args in edits:
				_edit_callables[args.pop_back()].callv(args)
			
			# During prepare, grid can't be read/write, so we block.
			Grid.step_prepare()
			# Can read, but not write to Grid now.
			_step_thread.start(_step, Thread.PRIORITY_NORMAL)

func find_cell_material(cell_material_name: StringName) -> CellMaterial:
	return cell_material_names[cell_material_name]

func find_cell_material_tag(tag: StringName) -> Array[CellMaterial]:
	return cell_material_tags[tag]

func add_grid_edit_method(m: Callable) -> int:
	assert(m.is_valid())
	var idx := _edit_callables.size()
	_edit_callables.push_back(m)
	return idx

func load_mods() -> void:
	unload_mods()
	
	# TODO: 
	is_server = true
	
	# Add ModEntry
	for mod_name in mod_names:
		var entry := load(ModEntry.entry_path(mod_name)) as ModEntry
		if entry:
			mod_entries.push_back(entry)
		else:
			push_error("ModEntry not found for ", mod_name)
	
	# Add CellMaterial
	for entry in mod_entries:
		if !entry.cell_materials:
			continue
		
		var root : Node = load(entry.cell_materials).instantiate()
		for cell_material : CellMaterial in root.get_children():
			root.remove_child(cell_material)
			cell_material.idx = cell_materials.size()
			cell_materials.push_back(cell_material)
		root.queue_free()
	
	# Add cell material tags
	for cell_material in cell_materials:
		Grid.add_cell_material(cell_material)
		
		for tag in cell_material.tags:
			if cell_material_tags.has(tag):
				cell_material_tags[tag].push_back(cell_material)
			else:
				cell_material_tags[tag] = [cell_material]
	
	# Add cell material name
	for cell_material in cell_materials:
		cell_material_names[cell_material.name] = cell_material
		# Add name to tags as well
		if cell_material_tags.has(cell_material.name):
			push_error("CellMaterial name is not unique: ", CellMaterial.name)
		cell_material_tags[cell_material.name] = [cell_material]
	
	# Create cell materials data base color atlas
	var base_color_images : Array[Image] = []
	for cell_material in cell_materials:
		if cell_material.base_color_image:
			base_color_images.push_back(cell_material.base_color_image)
		else:
			var img := Image.create(1, 1, false, Image.FORMAT_RGBA8)
			img.set_pixel(0, 0, cell_material.base_color)
			base_color_images.push_back(img)
	var new_img := ImagePacker.pack(
		base_color_images,
		Image.FORMAT_RGBA8,
		&"base_color_start")
	base_color_atlas_img.set_data(
		new_img.get_width(),
		new_img.get_height(),
		false,
		Image.FORMAT_RGBA8,
		new_img.get_data())
	for cell_material in cell_materials:
		cell_material.base_color_atlas_coord = base_color_images[cell_material.idx].get_meta(&"base_color_start")
		base_color_images[cell_material.idx].remove_meta(&"base_color_start")
	
	# Create cell materials base color rect
	var floats := PackedFloat32Array()
	floats.resize(cell_materials.size() * 4)
	for cell_material in cell_materials:
		floats[cell_material.idx * 4] = cell_material.base_color_atlas_coord.x
		floats[cell_material.idx * 4 + 1] = cell_material.base_color_atlas_coord.y
		var size := base_color_images[cell_material.idx].get_size()
		floats[cell_material.idx * 4 + 2] = size.x
		floats[cell_material.idx * 4 + 3] = size.y
	base_color_rect_img.set_data(
		cell_materials.size(),
		1,
		false,
		Image.FORMAT_RGBAF,
		floats.to_byte_array())
	
	# Create other cell materials data
	glow_img.set_data(1, 1, false, Image.FORMAT_RGBA8, [0, 0, 0, 0])
	glow_img.resize(cell_materials.size(), 1, Image.INTERPOLATE_NEAREST)
	light_modulate_img.set_data(1, 1, false, Image.FORMAT_RGBA8, [0, 0, 0, 0])
	light_modulate_img.resize(cell_materials.size(), 1, Image.INTERPOLATE_NEAREST)
	for cell_material in cell_materials:
		cell_material.set_glow(cell_material.glow)
		cell_material.set_light_modulate(cell_material.light_modulate)
	
	# TODO: Add cell reations
	
	# Add generation passes
	for entry in mod_entries:
		if !entry.generation_passes:
			continue
		
		var root : Node = load(entry.generation_passes).instantiate()
		for gen_pass : GenerationPass in root.get_children():
			root.remove_child(gen_pass)
			var gen_pass_idx := 0
			while true:
				if gen_pass_idx >= generation_passes.size():
					generation_passes.push_back(gen_pass)
					break
				elif generation_passes[gen_pass_idx].process_priority > gen_pass.process_priority:
					generation_passes.insert(gen_pass_idx, gen_pass)
					break
				gen_pass_idx += 1
		root.queue_free()
	for gen_pass in generation_passes:
		Grid.add_generation_pass(gen_pass)
	
	for entry in mod_entries:
		if entry.entry_script:
			if entry.entry_script.has_method(&"_entry"):
				entry.entry_script._entry()

func unload_mods() -> void:
	for entry in mod_entries:
		if entry.entry_script:
			if entry.entry_script.has_method(&"_exit"):
				entry.entry_script._exit()
	mod_entries = []
	
	Grid.clear()
	Grid.clear_generation_passes()
	Grid.clear_cell_reactions()
	Grid.clear_cell_materials()
	
	for cell_material in cell_materials:
		cell_material.queue_free()
	cell_materials = []
	cell_material_names = {}
	cell_material_tags = {}
	
	for gen_pass in generation_passes:
		gen_pass.queue_free()
	generation_passes = []
	
	_edit_callables = []
	_queued_edits = {}
	_next_edits = []

@rpc("authority", "call_remote", "reliable", 1)
func _edit_peer(tick: int, bytes: PackedByteArray) -> void:
	_queued_edits[tick] = bytes_to_var(bytes)

func _step() -> void:
	Grid.step()

func _on_img_changed(idx: int) -> void:
	_img_changed[idx][0] = true


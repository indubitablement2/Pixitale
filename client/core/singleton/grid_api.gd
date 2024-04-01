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
# which will be the same for all peers.

var is_server := true

var mod_names : Array[String] = ["core", "base"]

var mod_entries : Array[ModEntry] = []

@export var base_color_atlas : ImageAndTexture
@export var base_color_atlas_rect: ImageAndTexture
@export var glow: ImageAndTexture
@export var light_modulate: ImageAndTexture

var cell_materials : Array[CellMaterial] = []
## StringName : CellMaterial
var cell_material_names := {}
## StringName : Array[CellMaterial]
var cell_material_tags := {}

## StringName : CellReaction
var cell_reactions := {}

## Ordered in this order
## - process_priority
## - which mod they are part of
## - their scene node order
var generation_passes : Array[GenerationPass] = []
## slice_idx(int) : GenerationData
var _generation_data := {}

var _edit_callables : Array[Callable] = []
## tick(int) : Array of Array(args..., callback idx)
var _queued_edits := {}
## [args..., callable_idx(int)]
var _next_edits := []

var _delete_node : Node = null

## chunks to be updated next step.
var queue_step_chunk_rect : Array[Rect2i] = []
var _step_thread := Thread.new()
var _current_step_chunk_rect : Array[Rect2i] = []
## {chunk_coord(Vector2i) : to_be_generated(bool)}
var _chunk_to_generate := {}
## passe([column([x(int), ys([int])])])
var _passes : Array[Array] = [[], [], []]
var _current_pass_idx := 0

func _exit_tree() -> void:
	unload_mods()

func _process(_delta: float) -> void:
	if is_server && !_next_edits.is_empty():
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
			_step_prepare()
			# Can read, but not write to Grid now.
			_step_thread.start(_step, Thread.PRIORITY_HIGH)

func find_cell_material(cell_material_name: StringName) -> CellMaterial:
	return cell_material_names[cell_material_name]

func find_cell_material_tag(tag: StringName) -> Array[CellMaterial]:
	return cell_material_tags[tag]

func add_grid_edit_method(m: Callable) -> int:
	assert(m.is_valid())
	var idx := _edit_callables.size()
	_edit_callables.push_back(m)
	return idx

## Use Grid.set_seed first as GenerationPass depends on it.
func load_mods() -> void:
	unload_mods()
	_delete_node = Node.new()
	add_child(_delete_node)
	
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
		_delete_node.add_child(root)
		for cell_material : CellMaterial in root.get_children():
			cell_material.idx = cell_materials.size()
			cell_materials.push_back(cell_material)
	
	# Add cell material tags
	for cell_material in cell_materials:
		Grid.add_cell_material(cell_material)
		
		for tag in cell_material.tags:
			if cell_material_tags.has(tag):
				cell_material_tags[tag].push_back(cell_material)
			else:
				var arr : Array[CellMaterial] = [cell_material]
				cell_material_tags[tag] = arr
	
	# Add cell material name
	for cell_material in cell_materials:
		cell_material_names[cell_material.name] = cell_material
		# Add name to tags as well
		if cell_material_tags.has(cell_material.name):
			push_error("CellMaterial name is not unique: ", CellMaterial.name)
		var arr : Array[CellMaterial] = [cell_material]
		cell_material_tags[cell_material.name] = arr
	
	# Create cell materials data base color atlas
	var base_color_images : Array[Image] = []
	for cell_material in cell_materials:
		if cell_material.base_color_image:
			base_color_images.push_back(cell_material.base_color_image)
		else:
			var img := Image.create(1, 1, false, Image.FORMAT_RGBA8)
			img.set_pixel(0, 0, cell_material.base_color)
			base_color_images.push_back(img)
	base_color_atlas.img = ImagePacker.pack(
		base_color_images,
		Image.FORMAT_RGBA8,
		&"base_color_start")
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
	base_color_atlas_rect.img = Image.create_from_data(
		cell_materials.size(),
		1,
		false,
		Image.FORMAT_RGBAF,
		floats.to_byte_array())
	
	# Create other cell materials data
	glow.img = Image.create(cell_materials.size(), 1, false, Image.FORMAT_RGBA8)
	light_modulate.img = Image.create(cell_materials.size(), 1, false, Image.FORMAT_RGBA8)
	for cell_material in cell_materials:
		cell_material.set_glow(cell_material.glow)
		cell_material.set_light_modulate(cell_material.light_modulate)
	
	# Add cell reations
	for entry in mod_entries:
		if !entry.cell_reactions:
			continue
		
		var root : Node = load(entry.cell_reactions).instantiate()
		_delete_node.add_child(root)
		for cell_reaction : CellReaction in root.get_children():
			if cell_reactions.has(cell_reaction.name):
				push_error("Duplicate CellReaction name: ", cell_reaction.name)
				continue
			cell_reactions[cell_reaction.name] = cell_reaction
			cell_reaction.add()
	
	# Add generation passes
	for entry in mod_entries:
		if !entry.generation_passes:
			continue
		
		var root : Node = load(entry.generation_passes).instantiate()
		_delete_node.add_child(root)
		for gen_pass : GenerationPass in root.get_children():
			var gen_pass_idx := 0
			while true:
				if gen_pass_idx >= generation_passes.size():
					generation_passes.push_back(gen_pass)
					break
				elif generation_passes[gen_pass_idx].process_priority > gen_pass.process_priority:
					generation_passes.insert(gen_pass_idx, gen_pass)
					break
				gen_pass_idx += 1
	
	for entry in mod_entries:
		if entry.entry_script:
			if entry.entry_script.has_method(&"_entry"):
				entry.entry_script._entry()
	
	#Grid.print_internals()

func unload_mods() -> void:
	if _step_thread.is_started():
		_step_thread.wait_to_finish()
	
	for entry in mod_entries:
		if entry.entry_script:
			if entry.entry_script.has_method(&"_exit"):
				entry.entry_script._exit()
	mod_entries = []
	
	Grid.clear()
	Grid.clear_cell_reactions()
	Grid.clear_cell_materials()
	
	for data : GenerationData in _generation_data.values():
		data.free()
	_generation_data = {}
	
	if _delete_node:
		_delete_node.queue_free()
	
	cell_materials = []
	cell_material_names = {}
	cell_material_tags = {}
	
	cell_reactions = {}
	
	generation_passes = []
	
	_edit_callables = []
	_queued_edits = {}
	_next_edits = []
	
	queue_step_chunk_rect = []

@rpc("authority", "call_remote", "reliable", 1)
func _edit_peer(tick: int, bytes: PackedByteArray) -> void:
	_queued_edits[tick] = bytes_to_var(bytes)

func _step_prepare() -> void:
	Grid.set_tick(Grid.get_tick() + 1)
	
	for p in _passes:
		p.clear()
	
	_chunk_to_generate.clear()
	
	_current_step_chunk_rect.clear()
	var tmp := _current_step_chunk_rect
	_current_step_chunk_rect = queue_step_chunk_rect
	queue_step_chunk_rect = tmp
	
	# {x(int) : ys(Array[int])}
	var columns := {}
	
	for rect in _current_step_chunk_rect:
		# Create new chunks.
		for y_offset in rect.size.y + 2:
			for x_offset in rect.size.x + 2:
				var chunk_coord := Vector2i(
					rect.position.x - 1 + x_offset,
					rect.position.y - 1 + y_offset)
				if Grid.try_create_chunk(chunk_coord):
					_chunk_to_generate[chunk_coord] = true
		
		# Add chunk to passes.
		for y_offset in rect.size.y:
			for x_offset in rect.size.x:
				var chunk_coord := Vector2i(
					rect.position.x + x_offset,
					rect.position.y + y_offset)
				var pass_idx := Grid.mod_neg(chunk_coord.x, 3)
				if columns.has(chunk_coord.x):
					columns[chunk_coord.x].push_back(chunk_coord.y)
				else:
					var ys : Array[int] = [chunk_coord.y]
					columns[chunk_coord.x] = ys
					_passes[pass_idx].push_back([chunk_coord.x, ys])

func _step() -> void:
	Grid.pre_step()
	
	# Create new GenerationData.
	for rect in _current_step_chunk_rect:
		var slice_start := GenerationData.compute_slice_idx(rect.position.x - 10)
		var slice_end := GenerationData.compute_slice_idx(rect.position.x + rect.size.x + 10)
		for slice_idx_offset in slice_end + 1:
			var slice_idx := slice_start + slice_idx_offset
			if !_generation_data.has(slice_idx):
				_generate_slice(slice_idx)
	
	for i in 3:
		_current_pass_idx = i
		
		var group_id := WorkerThreadPool.add_group_task(
			_step_column,
			_passes[i].size())
		WorkerThreadPool.wait_for_group_task_completion(group_id)
	
	Grid.post_step()

func _step_column(column_idx: int) -> void:
	var x : int = _passes[_current_pass_idx][column_idx][0]
	var ys : Array[int] = _passes[_current_pass_idx][column_idx][1]
	
	ys.sort()
	ys.reverse()
	
	var y := Global.INT_MAX
	for maybe_y : int in ys:
		if y == maybe_y:
			continue
		y = maybe_y
		_step_chunk(Vector2i(x, y))

func _step_chunk(chunk_coord : Vector2i) -> void:
	# Generate new chunks.
	for y in 3:
		for x in 3:
			var other_chunk_coord := chunk_coord - Vector2i.ONE + Vector2i(x, y)
			if _chunk_to_generate.get(other_chunk_coord, false):
				_generate_chunk(other_chunk_coord)
				_chunk_to_generate[other_chunk_coord] = false
	
	Grid.step_chunk(chunk_coord)

func _generate_slice(slice_idx: int) -> void:
	# set_tick reset Grid's rng.
	var tick := Grid.get_tick()
	Grid.set_tick(0)
	
	var data := GenerationData.new(slice_idx)
	for gen_pass in generation_passes:
		gen_pass._generate_slice(data)
	_generation_data[slice_idx] = data
	
	Grid.set_tick(tick)

func _generate_chunk(chunk_coord: Vector2i) -> void:
	var iter := Grid.iter_chunk(chunk_coord)
	var data : GenerationData = _generation_data[GenerationData.compute_slice_idx(chunk_coord.x)]
	iter.fill_remaining(0)
	for gen_pass in generation_passes:
		iter.reset_iter()
		gen_pass._generate_chunk(iter, data)

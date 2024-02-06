extends Node
## Gdscript friendly api over Grid.

## Editing Grid safely:
## While it is safe to read from Grid at any time, it is not to modify it.
## Instead use queue_edit. This also help ensure determinism.
## Bad unless from queue_edit:
## Grid.set_tick(123)
## var iter = Grid.iter_rect(rect); iter.next(); iter.set_cell(0)
## Always OK:
## Grid.get_tick()
## var iter = Grid.iter_rect(rect); iter.next(); iter.get_cell()
## GridApi.queue_edit(&"my_method_to_edit_rect", [rect])

## Randomness and determinism:
## When generating chunk, use rand methods from GridChunkIter
## to ensure the same chunk is generated for all peers and
## regarless of time.
##
## When doing other deterministic edits, use rand methods from Grid
## which change over time, but will be the same for all peers.

var is_server := true

var mod_names : Array[String] = ["base"]

var mod_entries : Array[ModEntry] = []

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

var grid_edit_callables : Array[Callable] = []
## StringName : int
var grid_edit_idx := {}
## tick(int) : Array of Array(args, method id)
var queued_grid_edits := {}
var next_grid_edits := []

func find_cell_material(cell_material_name: StringName) -> CellMaterial:
	return cell_material_names[cell_material_name]

func find_cell_material_tag(tag: StringName) -> Array[CellMaterial]:
	return cell_material_tags[tag]

## Should only be called by the server.
## Method called from this are called in deterministic order and networked.
## It is only safe to edit the Grid through this.
## args can be almost anything but not Object and Callable.
## Uses var_to_bytes(args) internally.
func queue_edit(method: StringName, args: Array) -> void:
	assert(is_server)
	args.push_back(grid_edit_idx[method])
	next_grid_edits.push_back(args)

@rpc("authority", "call_remote", "reliable", 1)
func _queue_edit_peer(tick: int, bytes: PackedByteArray) -> void:
	queued_grid_edits[tick] = bytes_to_var(bytes)

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
	
	# Add empty cell material
	cell_materials.push_back(CellMaterial.new())
	
	# Add cell materials
	for entry in mod_entries:
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
	
	# Add cell materia name
	for cell_material in cell_materials:
		# Add name to tags as well
		if cell_material_tags.has(cell_material.name):
			push_error("CellMaterial name is not unique: ", CellMaterial.name)
		cell_material_tags[cell_material.name] = [cell_material]
		
		cell_material_names[cell_material.name] = cell_material
	
	# TODO: Add cell reations
	
	# Add generation passes
	for entry in mod_entries:
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
		entry.entry()

func unload_mods() -> void:
	for entry in mod_entries:
		entry.exit()
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

func _ready() -> void:
	#process_mode = Node.PROCESS_MODE_DISABLED
	pass

func _process(_delta: float) -> void:
	if is_server:
		# Send queued grid edits to peers
		if multiplayer.has_multiplayer_peer():
			_queue_edit_peer(Grid.get_tick(), var_to_bytes(next_grid_edits))
		
		queued_grid_edits[Grid.get_tick()] = next_grid_edits
		next_grid_edits = []
	
	# Step up to 2 times if behind
	for i in 2:
		if queued_grid_edits.has(Grid.get_tick()):
			var grid_edits := queued_grid_edits[Grid.get_tick()] as Array
			queued_grid_edits.erase(Grid.get_tick())
		
			# Only after calling this it safe to modify the grid.
			Grid.step_wait_to_finish()
			
			for args in grid_edits:
				grid_edit_callables[args.pop_back()].callv(args)
			
			Grid.step_prepare()


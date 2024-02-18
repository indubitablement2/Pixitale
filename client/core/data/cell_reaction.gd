extends Node
class_name CellReaction

## A reaction between cells.

## If extra logic is needed after a reaction trigger, 
## create a new script which extend CellReaction and add:
## func callback(coord: Vector2i) -> void:
## coord will be at in1.
## This will be called at the very end of step
## which gives full exclusive read/write access to Grid.

## Tag which can also be a name.
@export var in1_tag : StringName = &""
## Tag which can also be a name.
@export var in2_tag : StringName = &""
## When the reaction trigger, what should in1 becomes.
@export var out1_name : StringName = &"Empty"
## When the reaction trigger, what should in2 becomes.
@export var out2_name : StringName = &"Empty"
## Chance for the reaction to trigger per in1-in2 pair per tick.
## 0 probability should be avoided as the cells will stay active forever.
@export_range(0.0000001, 1.0, 0.0000001) var probability := 1.0

var reactions_id := PackedInt64Array()

func add() -> void:
	remove()
	
	var out1 : CellMaterial = GridApi.find_cell_material(out1_name)
	var out2 : CellMaterial = GridApi.find_cell_material(out2_name)
	if out1 && out2:
		var callback := Callable()
		if has_method(&"callback"):
			callback = Callable(self, &"callback")
		
		for in1 in GridApi.find_cell_material_tag(in1_tag):
			for in2 in GridApi.find_cell_material_tag(in2_tag):
				reactions_id.push_back(Grid.add_cell_reaction(
					in1.idx,
					in2.idx,
					out1.idx,
					out2.idx,
					probability,
					callback))

func remove() -> void:
	for reaction_id in reactions_id:
		Grid.remove_cell_reaction(reaction_id)
	
	reactions_id.clear()

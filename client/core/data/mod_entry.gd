extends Resource
class_name ModEntry

static func folder_path(mod_name: String) -> String:
	return "res://" + mod_name + "/"

static func entry_path(mod_name: String) -> String:
	return "res://" + mod_name + "/entry.tres"

@export_file("*.tscn") var cell_materials

@export_file("*.tscn") var generation_passes

## Static methods which are safe to edit Grid with.
## Methods are networked and called on remote peers.
## This also help ensure determinism
## (as long as methods are deterministic themself).
@export var grid_edits : GDScript = null

## Optional entry/exit script.
@export var entry_script : GDScript = null

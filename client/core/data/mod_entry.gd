extends Resource
class_name ModEntry

static func folder_path(mod_name: String) -> String:
	return "res://" + mod_name + "/"

static func entry_path(mod_name: String) -> String:
	return "res://" + mod_name + "/entry.tres"

@export_file("*.tscn") var cell_materials

@export_file("*.tscn") var cell_reactions

@export_file("*.tscn") var generation_passes

## Optional entry/exit script.
@export var entry_script : GDScript = null

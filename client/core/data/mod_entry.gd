extends Resource
class_name ModEntry

static func folder_path(mod_name: String) -> String:
	return "res://" + mod_name + "/"

static func entry_path(mod_name: String) -> String:
	return "res://" + mod_name + "/entry.tres"

@export_file("*.tscn") var cell_materials

@export_file("*.tscn") var generation_passes

## Called once before game start.
func entry() -> void:
	pass

## Called after game exit.
## Any change made by entry should be undone here.
func exit() -> void:
	pass

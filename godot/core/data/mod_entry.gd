extends Object
class_name ModEntry

static func entry_path(mod_name: String) -> String:
	return "res://" + mod_name + "/entry.gd"

# This will be called when loading this mod
# if it is located at "res://[mod_name]/entry.gd".
static func entry() -> void:
	pass

extends Node


## Called once after mod is added.
static func entry() -> void:
	print("base entry")

## Called before mod is removed.
## Any change made by entry should be undone here.
static func exit() -> void:
	print("base exit")

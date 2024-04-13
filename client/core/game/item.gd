extends Sprite2D
class_name ItemWorld

const PICK_UP_RANGE := 10.0 * 10.0
const STOP_PICK_UP_RANGE := 1000.0 * 1000.0
const PICK_UP_SPEED := 1000.0
const GRAVITY := 400.0

@onready var grid_body : GridBody = $GridBody
var item : ItemInstance
var attracted_by : Player

func _process(delta: float) -> void:
	if attracted_by:
		if position.distance_squared_to(attracted_by.position) > STOP_PICK_UP_RANGE:
			_remove_attracted_by()
		elif position.distance_squared_to(attracted_by.position) < PICK_UP_RANGE:
			attracted_by.add_item(item)
			queue_free()
		else:
			grid_body.collision = false
			var dir := position.direction_to(attracted_by.position)
			dir += dir.orthogonal() * sin(Global.time * 5.0) * 0.5
			grid_body.velocity += dir * PICK_UP_SPEED * delta
	else:
		grid_body.velocity.y += GRAVITY * delta
		grid_body.collision = true
	
	grid_body.move_and_slide()
	
	grid_body.velocity *= 0.9
	
	# TODO: sleep when chunk && is_on_floor

func set_item(ist: ItemInstance) -> void:
	item = ist
	texture = item.data.icon

func _remove_attracted_by() -> void:
	if attracted_by:
		attracted_by.tree_exiting.disconnect(_on_attracted_by_tree_exiting)
		attracted_by = null

func _on_detect_attractor_area_entered(area: Area2D) -> void:
	_remove_attracted_by()
	attracted_by = area.get_parent() as Player
	if attracted_by:
		attracted_by.tree_exiting.connect(_on_attracted_by_tree_exiting)

func _on_attracted_by_tree_exiting() -> void:
	attracted_by = null

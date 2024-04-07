extends Node2D
class_name Player

static var local_player : Player
## {peer_id(int) : Player}
static var _players := {}

@export var root_body : Node2D
@export var arm_back : Sprite2D
@export var hand_back : Node2D
@export var arm_front : Sprite2D

@export var animation_player : AnimationPlayer
@export var animation_tree : AnimationTree

@export var grid_body : GridBody

var movement_speed := 460.0
var air_control := 0.3
var air_friction := 0.38
var ground_friction := 0.04
var gravity := 400.0

var queue_jump := 0.0
var jump_cooldown := 0.0
## How long since we were last on floor.
var _last_on_floor := 0.0

var inventory : Array[ItemInstance] = []

func _ready() -> void:
	inventory.resize(5 * 9)
	
	_players[get_multiplayer_authority()] = self
	if is_multiplayer_authority():
		local_player = self

func _exit_tree() -> void:
	_players.erase(get_multiplayer_authority())

func _process(delta: float) -> void:
	arm_idle()
	
	if is_multiplayer_authority():
		_movement(delta)

func _movement(delta: float) -> void:
	if grid_body.is_on_floor():
		_last_on_floor = 0.0
	else:
		_last_on_floor += delta
	
	if Input.is_action_just_pressed(&"up"):
		queue_jump = 0.1
	
	# Floor jump.
	jump_cooldown -= delta
	queue_jump -= delta
	if queue_jump > 0.0 && _last_on_floor < 0.05 && jump_cooldown < 0.0:
		jump_cooldown = 0.05
		grid_body.velocity.y -= 200.0
		queue_jump = 0.0
		_set_jump_animation(true)
	else:
		_set_jump_animation(false)
	
	var wish_dir := (Input.get_action_strength(&"right") - Input.get_action_strength(&"left"))
	
	# Move horizontaly.
	if grid_body.is_on_floor():
		if is_zero_approx(wish_dir):
			grid_body.velocity.x -= clampf(
				grid_body.velocity.x,
				-movement_speed * delta,
				movement_speed * delta)
		else:
			grid_body.velocity.x += wish_dir * movement_speed * delta
	else:
		grid_body.velocity.x += wish_dir * movement_speed * delta * air_control
	
	grid_body.velocity.y += gravity * delta
	
	grid_body.move_and_slide()
	
	# Apply friction
	grid_body.velocity.y *= pow(air_friction, delta)
	if grid_body.is_on_floor():
		grid_body.velocity.x *= pow(
			ground_friction * GridApi.cell_materials[grid_body.get_floor_cell()].friction_multiplier,
			delta)
	else:
		grid_body.velocity.x *= pow(air_friction, delta)
	
	# Look toward wish movement dir.
	if wish_dir > 0.1:
		root_body.scale.x = 1.0
	elif wish_dir < -0.1:
		root_body.scale.x = -1.0
	
	_set_on_floor_animation(grid_body.is_on_floor())
	_set_run_animation(!is_zero_approx(wish_dir) && !is_zero_approx(grid_body.velocity.x) && signf(grid_body.velocity.x) == signf(wish_dir))

func _set_jump_animation(value: bool) -> void:
	animation_tree.set(&"parameters/conditions/jump", value)

func _set_run_animation(value: bool) -> void:
	animation_tree.set(&"parameters/conditions/run", value)
	animation_tree.set(&"parameters/conditions/not_run", !value)

func _set_on_floor_animation(value: bool) -> void:
	animation_tree.set(&"parameters/conditions/on_floor", value)
	animation_tree.set(&"parameters/conditions/not_on_floor", !value)

func arm_aim_mouse() -> void:
	arm_back.look_at(Global.mouse_position)

func arm_idle() -> void:
	arm_back.rotation = -arm_front.rotation

## Return the peer's Player or null if not found.
static func _get_peer_player(peer_id: int) -> Player:
	return _players.get(peer_id)


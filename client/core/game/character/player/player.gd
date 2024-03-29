extends Node2D
class_name Player

static var local_player : Player
## {peer_id(int) : Player}
static var _players := {}

@export var body : Sprite2D
@export var arm_back : Sprite2D
@export var hand_back : Node2D
@export var arm_front : Sprite2D
@export var hand_front : Node2D

@export var animation_player : AnimationPlayer
@export var grid_body : GridBody

var movement_speed := 460.0
var air_control := 0.3
var air_friction := 0.38
var ground_friction := 0.04
var gravity := 400.0

var queue_jump := 0.0
## How long since we were last on floor.
var last_on_floor := 0.0
var floor_jump_cooldown := 0.15

var inventory : Array[ItemInstance] = []

func _ready() -> void:
	inventory.resize(5 * 9)
	
	_players[get_multiplayer_authority()] = self
	if is_multiplayer_authority():
		local_player = self

func _exit_tree() -> void:
	_players.erase(get_multiplayer_authority())

var t := 0.0
func _process(delta: float) -> void:
	if is_multiplayer_authority():
		_movement(delta)
	
	#var wish_rot := 0.0
	#if body.animation == &"Run":
		#t += delta
		#arm_back.rotation = sin(t * movement_speed * 0.02) * 0.6 - 0.2
		#arm_front.rotation = -sin(t * movement_speed * 0.02) * 0.6
	#if body.animation == &"Air":
		#t += delta
		#arm_back.rotation = -0.8
		#arm_front.rotation = 0.6
	
	
	$Label.set_text(str(
		grid_body.is_on_floor(), "\n",
		grid_body.is_on_ceiling(), "\n",
		grid_body.is_on_left_wall(), "\n",
		grid_body.is_on_right_wall()))
	$Velocity.text = str(grid_body.velocity.round())

func _movement(delta: float) -> void:
	if grid_body.is_on_floor():
		last_on_floor = 0.0
	else:
		last_on_floor += delta
	
	if Input.is_action_just_pressed(&"up"):
		queue_jump = 0.1
	
	# Floor jump.
	floor_jump_cooldown -= delta
	if queue_jump > 0.0 && last_on_floor < 0.1 && floor_jump_cooldown < 0.0:
		floor_jump_cooldown = 0.1
		grid_body.velocity.y -= 200.0
		queue_jump = 0.0
	queue_jump -= delta
	
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
		
		#if absf(grid_body.velocity.x) > 50.0 && signf(grid_body.velocity.x) == signf(wish_dir):
			#body.animation = &"Run"
		#else:
			#body.animation = &"Idle"
	else:
		grid_body.velocity.x += wish_dir * movement_speed * delta * air_control
		#body.animation = &"Air"
	
	# Look toward wish movement dir.
	if wish_dir > 0.1:
		body.scale.x = 1.0
	elif wish_dir < -0.1:
		body.scale.x = -1.0
	
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

## Return the peer's Player or null if not found.
static func _get_peer_player(peer_id: int) -> Player:
	return _players.get(peer_id)

#@rpc("authority", "call_local")
#func jump() -> void:
	#pass



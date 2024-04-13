extends Node2D
class_name Player

static var local_player : Player
## {peer_id(int) : Player}
static var _players := {}

## Used to flip sprites, by changing scale.x.
@export var root_body : Node2D
## Arm which hold item.
@export var arm_back : Sprite2D
## Where weilded item should go.
@export var hand_back : Node2D
@export var arm_front : Sprite2D

@export var animation_player : AnimationPlayer
@export var animation_tree : AnimationTree

@export var grid_body : GridBody

@export var item_attractor : Area2D

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
var weilded_item_slot := 0 : set = set_weilded_item_slot

## Get the Player node from a weilded item scene.
static func weilded_item_get_player(node: Node) -> Player:
	return node.get_parent().get_parent().get_parent().get_parent().get_parent()

func _ready() -> void:
	inventory.resize(10)
	
	Game.spawn_item("res://base/item/sword_1.tres", Vector2(-100, 0), 1)
	
	_players[get_multiplayer_authority()] = self
	if is_multiplayer_authority():
		local_player = self

func _exit_tree() -> void:
	_players.erase(get_multiplayer_authority())

func _unhandled_input(event: InputEvent) -> void:
	if !is_multiplayer_authority():
		return
	
	if event.is_action_pressed("primary"):
		# TODO: check if use scn present.
		
		var item := inventory[weilded_item_slot]
		if !item:
			return
		# TODO: add use scn

func _process(delta: float) -> void:
	var item := inventory[weilded_item_slot]
	if item:
		if !item.data.control_arm_while_weilded:
			arm_idle()
	else:
		arm_idle()
	
	if is_multiplayer_authority():
		_movement(delta)

@rpc("authority", "call_local", "reliable")
func set_weilded_item_slot(value: int) -> void:
	while hand_back.get_child_count() > 0:
		hand_back.get_child(0).queue_free()
	
	weilded_item_slot = clampi(value, 0, inventory.size())
	
	var item := inventory[weilded_item_slot]
	if item:
		if item.data.weild_scene:
			hand_back.add_child(item.data.weild_scene.instantiate())

# TODO: Use item data id + qty (can not send resource through rpc)

func add_item(item: ItemInstance) -> void:
	if !is_multiplayer_authority():
		return
	
	var data_path := item.data.resource_path
	
	# Stack with same item.
	for slot in inventory.size():
		var i := inventory[slot]
		if !i:
			continue
		if i.data == item.data:
			var mv := i.quantity - mini(i.quantity + item.quantity, i.data.max_quantity)
			if mv > 0:
				i.quantity += mv
				item.quantity -= mv
				_add_item(data_path, mv, slot)
				if item.quantity <= 0:
					return
	
	# Add to first empty slot.
	for slot in inventory.size():
		var i := inventory[slot]
		if !i:
			inventory[slot] = item
			_add_item(data_path, item.quantity, slot)
			if slot == weilded_item_slot:
				set_weilded_item_slot(slot)
			return
	
	# Add to new slot.
	_add_item(data_path, item.quantity, inventory.size())
	inventory.push_back(item)

@rpc("authority", "call_remote", "reliable")
func _add_item(data_path: String, qty: int, slot: int) -> void:
	inventory.resize(maxi(inventory.size(), slot))
	inventory[slot] = load(data_path).make_instance(qty)

func consume_item(idx: int) -> void:
	pass

func drop_item(idx: int) -> void:
	pass

func remove_item(idx: int) -> void:
	pass

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

func _on_pickup_item_area_entered(area: Area2D) -> void:
	var item : ItemWorld = area.get_parent()
	print_debug("took item", item.item.data.display_name)
	add_item(item.item)
	item.queue_free()

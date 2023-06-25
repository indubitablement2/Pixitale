extends GridCharacterBody

const ACCELERATION = Vector2(8.0, 0.0)
const GRAVITY = 12.0

func _ready() -> void:
	GameGlobals.player_alive = true

func _exit_tree() -> void:
	GameGlobals.player_alive = false

func _physics_process(delta: float) -> void:
	var mouse_pos := get_global_mouse_position()
	var grid_pos := Vector2i(mouse_pos)
	var chunk_pos := grid_pos / Vector2i(32, 32)
	
	var dir := Vector2(
		Input.get_action_strength("right") - Input.get_action_strength("left"),
		Input.get_action_strength("down") - Input.get_action_strength("up")
	)
	
	if Input.is_action_just_pressed("up"):
		velocity.y -= 5.0
	
	velocity += dir * ACCELERATION * delta
	velocity.y += GRAVITY * delta
	velocity *= 0.96
	move()
	
	GameGlobals.player_position = position
	
	var v := (velocity * 10.0).round() * 0.1
	$velocity.set_text(str(v))
	
	$position.set_text(str(position.round()))
	
	$"chunk active".set_text(str(Grid.is_chunk_active(chunk_pos)))

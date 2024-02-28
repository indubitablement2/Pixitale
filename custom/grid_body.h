#ifndef GRID_BODY_H
#define GRID_BODY_H

#include "preludes.h"
#include "scene/2d/node_2d.h"

class GridBody : public Node2D {
	GDCLASS(GridBody, Node2D);

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	Vector2 half_size = Vector2(8.0f, 8.0f);
	void set_half_size(Vector2 value);
	Vector2 get_half_size() const;

	Vector2 velocity = Vector2(0.0f, 0.0f);
	void set_velocity(Vector2 value);
	Vector2 get_velocity() const;

	i32 max_step_height = 4;
	void set_max_step_height(i32 value);
	i32 get_max_step_height() const;

	// Smooth out moving up/down slopes.
	f32 step_offset = 0.0f;
	f32 step_smoothing = 0.75f;
	void set_step_smoothing(f32 value);
	f32 get_step_smoothing() const;

	bool draw_half_size = false;
	void set_draw_half_size(bool value);
	bool get_draw_half_size() const;

	// Stick to floor when was_on_floor && velocity.y <= 0.0
	bool stick_to_floor = true;
	void set_stick_to_floor(bool value);
	bool get_stick_to_floor() const;

	bool collision_enabled = true;
	void set_collision_enabled(bool value);
	bool get_collision_enabled() const;

	bool was_on_floor = false;
	bool get_was_on_floor() const;
	bool is_on_floor = false;
	bool get_is_on_floor() const;
	bool is_on_ceiling = false;
	bool get_is_on_ceiling() const;
	bool is_on_left_wall = false;
	bool get_is_on_left_wall() const;
	bool is_on_right_wall = false;
	bool get_is_on_right_wall() const;
	bool get_is_on_wall() const;

	void move_and_slide();

	u32 get_floor_cell() const;
};

#endif
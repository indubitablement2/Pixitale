#ifndef GRID_BODY_H
#define GRID_BODY_H

#include "preludes.h"
#include "scene/2d/node_2d.h"

class GridBody : public Node2D {
	GDCLASS(GridBody, Node2D);

private:
	f32 step_offset = 0.0f;

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	Vector2 half_size = Vector2(10.0f, 10.0f);
	void set_half_size(Vector2 value);
	Vector2 get_half_size() const;

	Vector2 velocity = Vector2(0.0f, 0.0f);
	void set_velocity(Vector2 value);
	Vector2 get_velocity() const;

	bool is_on_floor = false;
	bool hit_ceiling = false;
	bool hit_left_wall = false;
	bool hit_right_wall = false;

	i32 max_step_height = 4;
	void set_max_step_height(i32 value);
	i32 get_max_step_height() const;

	bool stick_to_floor = true;

	bool collision = true;

	void move_and_slide();
};

#endif
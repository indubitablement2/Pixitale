#include "grid_character_body.h"

#include "cell.hpp"
#include "cell_material.h"
#include "grid.h"

bool is_blocking(i32 x, i32 y) {
	u32 cell = Grid::get_cell_checked(x, y);
	u32 mat_idx = Cell::material_idx(cell);

	if (mat_idx == 0) {
		// Empty cell.
		return false;
	}

	auto mat_ptr = CellMaterial::materials[mat_idx];
	if (mat_ptr.collision == Cell::Collision::COLLISION_SOLID) {
		return true;
	}

	return false;
}

bool is_row_blocked(
		i32 left,
		i32 right,
		i32 y) {
	for (i32 x = left; x <= right; x++) {
		if (is_blocking(x, y)) {
			return true;
		}
	}

	return false;
}

bool block_or_step_left(
		i32 &left,
		i32 &right,
		i32 &top,
		i32 &bot,
		Vector2 &new_position,
		f32 &step_offset,
		i32 max_steps_height) {
	bool blocked = false;

	i32 floor = (bot - top) + 1;
	for (i32 y = top; y <= bot; y++) {
		floor--;

		if (is_blocking(left, y)) {
			if (floor <= max_steps_height) {
				// Try to step up.
				for (i32 y_step = 1; y_step <= floor; y_step++) {
					for (i32 x = left; x < right; x++) {
						if (is_blocking(x, top - y_step)) {
							blocked = true;
							break;
						}
					}
					if (blocked) {
						break;
					}
				}

				if (!blocked) {
					top -= floor;
					bot -= floor;
					f32 pre_step_y = new_position.y;
					new_position.y = std::floor(new_position.y - f32(floor)) - 0.02f;

					step_offset -= new_position.y - pre_step_y;
				}
			} else {
				blocked = true;
			}

			break;
		}
	}

	return blocked;
}

bool block_or_step_right(
		i32 &left,
		i32 &right,
		i32 &top,
		i32 &bot,
		Vector2 &new_position,
		f32 &step_offset,
		i32 max_steps_height) {
	bool blocked = false;

	i32 floor = (bot - top) + 1;
	for (i32 y = top; y <= bot; y++) {
		floor--;

		if (is_blocking(right, y)) {
			if (floor <= max_steps_height) {
				// Try to step up.
				for (i32 y_step = 1; y_step <= floor; y_step++) {
					for (i32 x = left + 1; x <= right; x++) {
						if (is_blocking(x, top - y_step)) {
							blocked = true;
							break;
						}
					}
					if (blocked) {
						break;
					}
				}

				if (!blocked) {
					top -= floor;
					bot -= floor;
					f32 pre_step_y = new_position.y;
					new_position.y = std::floor(new_position.y - f32(floor)) - 0.02f;

					step_offset -= new_position.y - pre_step_y;
				}
			} else {
				blocked = true;
			}

			break;
		}
	}

	return blocked;
}

void GridCharacterBody::_notification(i32 p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (Engine::get_singleton()->is_editor_hint()) {
				draw_rect(
						Rect2(size * -0.5f, size),
						Color(1.0f, 0.0f, 0.0f, 0.5f),
						false);
			}
		} break;
	}
}

void GridCharacterBody::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("set_size", "value"),
			&GridCharacterBody::set_size);
	ClassDB::bind_method(
			D_METHOD("get_size"),
			&GridCharacterBody::get_size);
	ADD_PROPERTY(
			PropertyInfo(Variant::VECTOR2,
					"size"),
			"set_size",
			"get_size");

	ClassDB::bind_method(
			D_METHOD("set_velocity", "value"),
			&GridCharacterBody::set_velocity);
	ClassDB::bind_method(
			D_METHOD("get_velocity"),
			&GridCharacterBody::get_velocity);
	ADD_PROPERTY(
			PropertyInfo(Variant::VECTOR2,
					"velocity"),
			"set_velocity",
			"get_velocity");

	ClassDB::bind_method(
			D_METHOD("set_max_steps_height", "value"),
			&GridCharacterBody::set_max_steps_height);
	ClassDB::bind_method(
			D_METHOD("get_max_steps_height"),
			&GridCharacterBody::get_max_steps_height);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT,
					"max_steps_height"),
			"set_max_steps_height",
			"get_max_steps_height");

	ClassDB::bind_method(
			D_METHOD("move"),
			&GridCharacterBody::move);
}

void GridCharacterBody::set_size(Vector2 value) {
	size = value.abs();

	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

Vector2 GridCharacterBody::get_size() const {
	return size;
}

void GridCharacterBody::set_velocity(Vector2 value) {
	velocity = value;
}

Vector2 GridCharacterBody::get_velocity() const {
	return velocity;
}

void GridCharacterBody::set_max_steps_height(i32 value) {
	max_steps_height = value;
}

i32 GridCharacterBody::get_max_steps_height() const {
	return max_steps_height;
}

void GridCharacterBody::move() {
	auto previous_position = get_position();
	previous_position.y -= step_offset;

	// TODO: Decrease faster when too high.
	step_offset *= 0.65f;
	// step_offset *= std::pow(0.95f, std::abs(step_offset) + 1);
	// if (std::abs(step_offset) > 4.0f) {
	// step_offset *= 0.8f;
	// }

	if (!collision) {
		set_position(previous_position + velocity + Vector2(0.0, step_offset));
		return;
	}

	auto new_position = previous_position;

	i32 top = previous_position.y - size.y * 0.5f;
	i32 bot = previous_position.y + size.y * 0.5f;
	i32 left = previous_position.x - size.x * 0.5f;
	i32 right = previous_position.x + size.x * 0.5f;

	hit_left_wall = false;
	hit_right_wall = false;
	hit_ceiling = false;

	bool was_on_floor = is_on_floor;
	is_on_floor = false;

	i32 steps = 0;
	f32 wish_horizontal_position = previous_position.x + velocity.x;

	// Horizontal movement.
	if (velocity.x < -0.01f) {
		// Move left until we hit a wall.
		while (new_position.x > wish_horizontal_position) {
			if (block_or_step_left(
						left,
						right,
						top,
						bot,
						new_position,
						step_offset,
						max_steps_height)) {
				hit_left_wall = true;

				if (steps > 0) {
					new_position.x = std::floor(new_position.x + 1.0f) + 0.02f;
				}

				velocity.x = 0.0f;
				break;
			} else {
				left -= 1;
				right -= 1;
				new_position.x -= 1.0f;

				steps++;
			}
		}

		if (!hit_left_wall) {
			if (block_or_step_left(
						left,
						right,
						top,
						bot,
						new_position,
						step_offset,
						max_steps_height)) {
				hit_left_wall = true;
				new_position.x = std::floor(new_position.x + 1.0f) + 0.02f;
				velocity.x = 0.0f;
			}
		}

		new_position.x = std::max(new_position.x, wish_horizontal_position);
	} else if (velocity.x > 0.01f) {
		// Move right until we hit a wall.
		while (new_position.x < wish_horizontal_position) {
			if (block_or_step_right(
						left,
						right,
						top,
						bot,
						new_position,
						step_offset,
						max_steps_height)) {
				hit_right_wall = true;

				if (steps > 0) {
					new_position.x = std::floor(new_position.x) - 0.02f;
				}

				velocity.x = 0.0f;
				break;
			} else {
				left += 1;
				right += 1;
				new_position.x += 1.0f;

				steps++;
			}
		}

		if (!hit_right_wall) {
			if (block_or_step_right(
						left,
						right,
						top,
						bot,
						new_position,
						step_offset,
						max_steps_height)) {
				hit_right_wall = true;
				new_position.x = std::floor(new_position.x) - 0.02f;
				velocity.x = 0.0f;
			}
		}

		new_position.x = std::min(new_position.x, wish_horizontal_position);
	}

	top = i32(new_position.y - size.y * 0.5f) - 1;
	bot = i32(new_position.y + size.y * 0.5f) + 1;
	left = i32(new_position.x - size.x * 0.5f) + 1;
	right = i32(new_position.x + size.x * 0.5f) - 1;

	f32 wish_vertical_position = new_position.y + velocity.y;

	// Vertical movement.
	if (velocity.y < -0.01f) {
		// Move up until we hit a ceiling.
		while (new_position.y > wish_vertical_position) {
			if (is_row_blocked(
						left,
						right,
						top)) {
				velocity.y = 0.0f;
				new_position.y = std::floor(new_position.y) + 0.02f;
				hit_ceiling = true;
				break;
			} else {
				top -= 1;
				new_position.y -= 1.0f;
			}
		}

		new_position.y = std::max(new_position.y, wish_vertical_position);
	} else {
		// Move down until we hit a floor.
		while (new_position.y < wish_vertical_position) {
			if (is_row_blocked(
						left,
						right,
						bot)) {
				velocity.y = 0.0f;
				new_position.y = std::floor(new_position.y + 1.0f) - 0.02f;
				is_on_floor = true;
				break;
			} else {
				bot += 1;
				new_position.y += 1.0f;
			}
		}

		new_position.y = std::min(new_position.y, wish_vertical_position);
	}

	if (!is_on_floor && was_on_floor && stick_to_floor && velocity.y > 0.02f) {
		bot = i32(new_position.y + size.y * 0.5f) + 1;

		// Move down until we hit a floor.
		for (i32 i = 0; i <= max_steps_height; i++) {
			if (is_row_blocked(
						left,
						right,
						bot)) {
				velocity.y = 0.0f;
				f32 pre_step_y = new_position.y;
				new_position.y = std::floor(new_position.y + 1.0f) - 0.02f;
				step_offset -= new_position.y - pre_step_y;
				is_on_floor = true;
				break;
			} else {
				bot += 1;
				new_position.y += 1.0f;
				step_offset -= 1.0f;
			}
		}

		if (!is_on_floor) {
			new_position.y -= f32(max_steps_height);
			step_offset += f32(max_steps_height);
		}
	}

	if (is_on_floor && stick_to_floor) {
		velocity.y = 0.0f;
	}

	new_position.y += step_offset;
	set_position(new_position);
}

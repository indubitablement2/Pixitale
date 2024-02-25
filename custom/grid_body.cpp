#include "grid_body.h"

#include "cell.hpp"
#include "cell_material.h"
#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/typedefs.h"
#include "grid.h"
#include "preludes.h"

const f32 SMALL_VALUE = 0.04f;
const f32 VERY_SMALL_VALUE = 0.002f;

// Coords are relative to top left cell of top left chunk.
struct GridBodyApi {
	Vector2 true_pos;
	Vector2 velocity;
	Vector2 wish_move;

	f32 top;
	f32 bot;
	f32 left;
	f32 right;

	i32 start;
	i32 end;

	Vector2 originf;
	Vector2i origini;

	Vector2i chunks_size;
	Chunk **chunks;

	inline GridBodyApi(Vector2 pos, Vector2 vel, Vector2 half_size, f32 max_step_height) :
			true_pos(pos),
			velocity(vel),
			wish_move(vel) {
		Rect2 rect = Rect2(pos - half_size, half_size * 2.0f);
		rect = rect.merge(Rect2((pos + vel) - half_size, half_size * 2.0f));

		rect.grow_by(4.0f);
		rect = rect.grow_side(SIDE_TOP, Math::abs(vel.x) * max_step_height);

		Vector2i chunks_start = div_floor(Vector2i(rect.position.floor()), 32);
		Vector2i chunks_end = div_floor(Vector2i((rect.position + rect.size).floor()), 32) + Vector2i(1, 1);

		origini = chunks_start * 32;

		chunks_size = chunks_end - chunks_start;
		chunks = new Chunk *[chunks_size.x * chunks_size.y];

		for (i32 y = 0; y < chunks_size.y; y++) {
			for (i32 x = 0; x < chunks_size.x; x++) {
				chunks[y * chunks_size.x + x] = Grid::get_chunk(chunks_start + Vector2i(x, y));
			}
		}

		originf = Vector2(origini);
		top = pos.y - half_size.y - originf.y;
		bot = pos.y + half_size.y - originf.y;
		left = pos.x - half_size.x - originf.x;
		right = pos.x + half_size.x - originf.x;
	}

	// inline GridBodyApi(GridBodyApi &other) :
	// 		chunks_start(other.chunks_start),
	// 		chunks_size(other.chunks_size),
	// 		chunks(other.chunks) {}

	inline void del() {
		delete[] chunks;
	}

	inline bool is_blocked(Vector2i coord, u32 collision_bitmask) {
		TEST_ASSERT(coord.x >= 0, "oob");
		TEST_ASSERT(coord.y >= 0, "oob");
		TEST_ASSERT(coord.x < chunks_size.x * 32, "oob");
		TEST_ASSERT(coord.y < chunks_size.y * 32, "oob");

		Chunk *chunk = chunks[(coord.y >> 5) * chunks_size.x + (coord.x >> 5)];

		if (chunk == nullptr) {
			return false;
		}

		u32 cell = chunk->get_cell(Vector2i(coord.x & 5, coord.y & 5));
		u32 mat_idx = Cell::material_idx(cell);

		if (mat_idx == 0) {
			// Empty cell.
			return false;
		}

		CellMaterial &mat = Grid::get_cell_material(mat_idx);
		return (u32(mat.collision) & collision_bitmask) != 0;
	}

	inline bool is_row_blocked(i32 y, u32 collision_bitmask) {
		// todo: could be faster
		for (i32 x = start; x < end; x++) {
			if (is_blocked(Vector2i(x, y), collision_bitmask)) {
				return true;
			}
		}

		return false;
	}

	// Return the number of cells from end. Eg. the step height.
	inline i32 is_column_blocked(i32 x, u32 collision_bitmask) {
		for (i32 y = start; y < end; y++) {
			if (is_blocked(Vector2i(x, y), collision_bitmask)) {
				return end - y;
			}
		}

		return 0;
	}

	inline bool is_up_blocked() {
		return false;
	}

	inline bool is_down_blocked() {
		return false;
	}

	inline i32 is_left_blocked() {
		return 0;
	}

	inline i32 is_right_blocked() {
		return 0;
	}

	inline bool clamp_down() {
		start = i32(Math::floor(left));
		end = i32(Math::ceil(right));

		f32 new_bot = Math::ceil(bot) - SMALL_VALUE;
		f32 dif = new_bot - bot;

		bool done = false;
		if (dif > wish_move.y) {
			dif = wish_move.y;
			done = true;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot += dif;
		top += dif;

		return done;
	}

	inline bool step_down() {
		f32 dif = 1.0f;
		bool done = false;
		if (dif > wish_move.y) {
			dif = wish_move.y;
			done = true;
		}

		f32 new_bot = bot + dif;
		if (is_row_blocked(
					i32(Math::floor(new_bot)),
					CellCollision::CELL_COLLISION_SOLID)) {
			velocity.y = 0.0f;
			return true;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot = new_bot;
		top += dif;

		return done;
	}

	inline void move(f32 x, f32 y) {
		left += x;
		right += x;
		top += y;
		bot += y;
	}

	inline void move_up() {
	}

	inline void move_down() {
	}

	inline void move_left() {
	}

	inline void move_right() {
	}

	inline bool step_right() {
		return false;
	}
};

bool is_row_blocked(const i32 start, i32 lenght, const i32 y, const bool blocking_platform) {
	i32 collision_bitmask = CellCollision::CELL_COLLISION_SOLID;
	if (blocking_platform) {
		collision_bitmask |= CellCollision::CELL_COLLISION_PLATFORM;
	}

	ChunkLocalCoord coord = ChunkLocalCoord(Vector2i(start, y));

	while (lenght > 0) {
		Chunk *chunk = Grid::get_chunk(coord.chunk_coord);
		i32 inc = MIN(32 - coord.local_coord.x, lenght);
		lenght -= inc;
		if (chunk == nullptr) {
			continue;
		}

		u32 *cell_ptr = chunk->get_cell_ptr(coord.local_coord);
		for (i32 i = 0; i < inc; i++) {
			TEST_ASSERT(coord.local_coord.x + i < 32, "bug");

			u32 mat_idx = Cell::material_idx(*(cell_ptr + i));
			if (mat_idx == 0) {
				continue;
			}

			auto mat = Grid::get_cell_material(mat_idx);
			if (mat.collision & collision_bitmask) {
				return true;
			}
		}

		coord.chunk_coord.x += 1;
		coord.local_coord.x = 0;
	}

	return false;
}

// Return the number of remaining cells to the collision.
i32 is_column_blocked(const i32 start, i32 lenght, const i32 x) {
	ChunkLocalCoord coord = ChunkLocalCoord(Vector2i(x, start));

	while (lenght > 0) {
		Chunk *chunk = Grid::get_chunk(coord.chunk_coord);
		i32 inc = MIN(32 - coord.local_coord.y, lenght);
		lenght -= inc;
		if (chunk == nullptr) {
			continue;
		}

		u32 *cell_ptr = chunk->get_cell_ptr(coord.local_coord);
		for (i32 i = 0; i < inc; i++) {
			TEST_ASSERT(coord.local_coord.y + i < 32, "bug");

			u32 mat_idx = Cell::material_idx(*(cell_ptr + i * 32));
			if (mat_idx == 0) {
				continue;
			}

			auto mat = Grid::get_cell_material(mat_idx);
			if (mat.collision == CellCollision::CELL_COLLISION_SOLID) {
				return lenght + inc - i;
			}
		}

		coord.chunk_coord.y += 1;
		coord.local_coord.y = 0;
	}

	return 0;
}

// bool is_blocking(i32 x, i32 y) {
// 	u32 cell = Grid::get_cell_checked(x, y);
// 	u32 mat_idx = Cell::material_idx(cell);

// 	if (mat_idx == 0) {
// 		// Empty cell.
// 		return false;
// 	}

// 	auto mat_ptr = CellMaterial::materials[mat_idx];
// 	if (mat_ptr.collision == Cell::Collision::COLLISION_SOLID) {
// 		return true;
// 	}

// 	return false;
// }

// bool is_row_blocked(
// 		i32 left,
// 		i32 right,
// 		i32 y) {
// 	for (i32 x = left; x <= right; x++) {
// 		if (is_blocking(x, y)) {
// 			return true;
// 		}
// 	}

// 	return false;
// }

// bool block_or_step_left(
// 		i32 &left,
// 		i32 &right,
// 		i32 &top,
// 		i32 &bot,
// 		Vector2 &new_position,
// 		f32 &step_offset,
// 		i32 max_steps_height) {
// 	bool blocked = false;

// 	i32 floor = (bot - top) + 1;
// 	for (i32 y = top; y <= bot; y++) {
// 		floor--;

// 		if (is_blocking(left, y)) {
// 			if (floor <= max_steps_height) {
// 				// Try to step up.
// 				for (i32 y_step = 1; y_step <= floor; y_step++) {
// 					for (i32 x = left; x < right; x++) {
// 						if (is_blocking(x, top - y_step)) {
// 							blocked = true;
// 							break;
// 						}
// 					}
// 					if (blocked) {
// 						break;
// 					}
// 				}

// 				if (!blocked) {
// 					top -= floor;
// 					bot -= floor;
// 					f32 pre_step_y = new_position.y;
// 					new_position.y = std::floor(new_position.y - f32(floor)) - 0.02f;

// 					step_offset -= new_position.y - pre_step_y;
// 				}
// 			} else {
// 				blocked = true;
// 			}

// 			break;
// 		}
// 	}

// 	return blocked;
// }

// bool block_or_step_right(
// 		i32 &left,
// 		i32 &right,
// 		i32 &top,
// 		i32 &bot,
// 		Vector2 &new_position,
// 		f32 &step_offset,
// 		i32 max_steps_height) {
// 	bool blocked = false;

// 	i32 floor = (bot - top) + 1;
// 	for (i32 y = top; y <= bot; y++) {
// 		floor--;

// 		if (is_blocking(right, y)) {
// 			if (floor <= max_steps_height) {
// 				// Try to step up.
// 				for (i32 y_step = 1; y_step <= floor; y_step++) {
// 					for (i32 x = left + 1; x <= right; x++) {
// 						if (is_blocking(x, top - y_step)) {
// 							blocked = true;
// 							break;
// 						}
// 					}
// 					if (blocked) {
// 						break;
// 					}
// 				}

// 				if (!blocked) {
// 					top -= floor;
// 					bot -= floor;
// 					f32 pre_step_y = new_position.y;
// 					new_position.y = std::floor(new_position.y - f32(floor)) - 0.02f;

// 					step_offset -= new_position.y - pre_step_y;
// 				}
// 			} else {
// 				blocked = true;
// 			}

// 			break;
// 		}
// 	}

// 	return blocked;
// }

void GridBody::_notification(i32 p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (Engine::get_singleton()->is_editor_hint()) {
				draw_rect(
						Rect2(-half_size, half_size * 2.0f),
						Color(1.0f, 0.0f, 0.0f, 0.5f),
						false);
			}
		} break;
	}
}

void GridBody::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("set_half_size", "value"),
			&GridBody::set_half_size);
	ClassDB::bind_method(
			D_METHOD("get_half_size"),
			&GridBody::get_half_size);
	ADD_PROPERTY(
			PropertyInfo(Variant::VECTOR2,
					"half_size"),
			"set_half_size",
			"get_half_size");

	ClassDB::bind_method(
			D_METHOD("set_velocity", "value"),
			&GridBody::set_velocity);
	ClassDB::bind_method(
			D_METHOD("get_velocity"),
			&GridBody::get_velocity);
	ADD_PROPERTY(
			PropertyInfo(Variant::VECTOR2,
					"velocity"),
			"set_velocity",
			"get_velocity");

	ClassDB::bind_method(
			D_METHOD("set_max_step_height", "value"),
			&GridBody::set_max_step_height);
	ClassDB::bind_method(
			D_METHOD("get_max_step_height"),
			&GridBody::get_max_step_height);
	ADD_PROPERTY(
			PropertyInfo(Variant::INT,
					"max_step_height"),
			"set_max_step_height",
			"get_max_step_height");

	ClassDB::bind_method(
			D_METHOD("move_and_slide"),
			&GridBody::move_and_slide);
}

void GridBody::set_half_size(Vector2 value) {
	half_size = value.abs();

	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

Vector2 GridBody::get_half_size() const {
	return half_size;
}

void GridBody::set_velocity(Vector2 value) {
	velocity = value;
}

Vector2 GridBody::get_velocity() const {
	return velocity;
}

void GridBody::set_max_step_height(i32 value) {
	max_step_height = value;
}

i32 GridBody::get_max_step_height() const {
	return max_step_height;
}

void GridBody::move_and_slide() {
	f32 dt = get_process_delta_time();

	GridBodyApi api = GridBodyApi(get_position(), velocity * dt, half_size, i32(max_step_height));

	if (api.wish_move.y > VERY_SMALL_VALUE) {
		// Move down
		if (!api.clamp_down()) {
			while (api.wish_move.y > VERY_SMALL_VALUE) {
				if (api.step_down()) {
					break;
				}
			}
		}
	}

	set_position(api.true_pos);

	api.del();
}

// void GridBody::move_and_slide() {
// 	f32 dt = get_process_delta_time();

// 	Vector2 previous_position = get_position();
// 	previous_position.y -= step_offset;

// 	// TODO: Decrease faster when too high.
// 	step_offset *= 0.65f;
// 	// step_offset *= std::pow(0.95f, std::abs(step_offset) + 1);
// 	// if (std::abs(step_offset) > 4.0f) {
// 	// step_offset *= 0.8f;
// 	// }

// 	// TODO: friction
// 	velocity *= Math::pow(0.5f, dt);

// 	if (!collision) {
// 		set_position(previous_position + Vector2(0.0, step_offset) + velocity * dt);
// 		return;
// 	}

// 	Vector2 new_position = previous_position;

// 	// i32 top = i32(Math::floor(previous_position.y - half_size.y));
// 	// i32 bot = i32(Math::floor(previous_position.y + half_size.y));
// 	// i32 left = i32(Math::floor(previous_position.x - half_size.x));
// 	// i32 right = i32(Math::floor(previous_position.x + half_size.x));

// 	hit_left_wall = false;
// 	hit_right_wall = false;
// 	hit_ceiling = false;

// 	bool was_on_floor = is_on_floor;
// 	is_on_floor = false;

// 	// i32 steps = 0;
// 	f32 wish_horizontal_position = previous_position.x + velocity.x;

// 	// Horizontal movement.
// 	if (velocity.x < -0.01f) {
// 		i32 top = i32(Math::floor(previous_position.y - half_size.y));
// 		f32 bot = i32(Math::floor(previous_position.y + half_size.y * 2.0f));
// 		i32 lenght = bot - top + 1;
// 		i32 left = i32(Math::floor(previous_position.x - half_size.x));

// 		// Move left.
// 		while (new_position.x > wish_horizontal_position) {
// 			i32 rem = is_column_blocked(top, lenght, left);
// 			if (rem == 0) {
// 				left -= 1;
// 				new_position.x -= 1.0f;
// 			} else if (rem <= max_steps_height) {
// 				is_row_blocked(const i32 start, i32 lenght, const i32 y, false)
// 			} else {
// 				new_position.x = std::floor(new_position.x + 1.0f) + 0.02f;
// 				velocity.x = 0.0f;
// 				break;
// 			}

// 			if (block_or_step_left(
// 						left,
// 						right,
// 						top,
// 						bot,
// 						new_position,
// 						step_offset,
// 						max_steps_height)) {
// 				hit_left_wall = true;

// 				if (steps > 0) {
// 					new_position.x = std::floor(new_position.x + 1.0f) + 0.02f;
// 				}

// 				velocity.x = 0.0f;
// 				break;
// 			} else {
// 				left -= 1;
// 				right -= 1;
// 				new_position.x -= 1.0f;

// 				steps++;
// 			}
// 		}

// 		if (!hit_left_wall) {
// 			if (block_or_step_left(
// 						left,
// 						right,
// 						top,
// 						bot,
// 						new_position,
// 						step_offset,
// 						max_steps_height)) {
// 				hit_left_wall = true;
// 				new_position.x = std::floor(new_position.x + 1.0f) + 0.02f;
// 				velocity.x = 0.0f;
// 			}
// 		}

// 		new_position.x = std::max(new_position.x, wish_horizontal_position);
// 	} else if (velocity.x > 0.01f) {
// 		// Move right until we hit a wall.
// 		while (new_position.x < wish_horizontal_position) {
// 			if (block_or_step_right(
// 						left,
// 						right,
// 						top,
// 						bot,
// 						new_position,
// 						step_offset,
// 						max_steps_height)) {
// 				hit_right_wall = true;

// 				if (steps > 0) {
// 					new_position.x = std::floor(new_position.x) - 0.02f;
// 				}

// 				velocity.x = 0.0f;
// 				break;
// 			} else {
// 				left += 1;
// 				right += 1;
// 				new_position.x += 1.0f;

// 				steps++;
// 			}
// 		}

// 		if (!hit_right_wall) {
// 			if (block_or_step_right(
// 						left,
// 						right,
// 						top,
// 						bot,
// 						new_position,
// 						step_offset,
// 						max_steps_height)) {
// 				hit_right_wall = true;
// 				new_position.x = std::floor(new_position.x) - 0.02f;
// 				velocity.x = 0.0f;
// 			}
// 		}

// 		new_position.x = std::min(new_position.x, wish_horizontal_position);
// 	}

// 	top = i32(new_position.y - size.y * 0.5f) - 1;
// 	bot = i32(new_position.y + size.y * 0.5f) + 1;
// 	left = i32(new_position.x - size.x * 0.5f) + 1;
// 	right = i32(new_position.x + size.x * 0.5f) - 1;

// 	f32 wish_vertical_position = new_position.y + velocity.y;

// 	// Vertical movement.
// 	if (velocity.y < -0.01f) {
// 		// Move up until we hit a ceiling.
// 		while (new_position.y > wish_vertical_position) {
// 			if (is_row_blocked(
// 						left,
// 						right,
// 						top)) {
// 				velocity.y = 0.0f;
// 				new_position.y = std::floor(new_position.y) + 0.02f;
// 				hit_ceiling = true;
// 				break;
// 			} else {
// 				top -= 1;
// 				new_position.y -= 1.0f;
// 			}
// 		}

// 		new_position.y = std::max(new_position.y, wish_vertical_position);
// 	} else {
// 		// Move down until we hit a floor.
// 		while (new_position.y < wish_vertical_position) {
// 			if (is_row_blocked(
// 						left,
// 						right,
// 						bot)) {
// 				velocity.y = 0.0f;
// 				new_position.y = std::floor(new_position.y + 1.0f) - 0.02f;
// 				is_on_floor = true;
// 				break;
// 			} else {
// 				bot += 1;
// 				new_position.y += 1.0f;
// 			}
// 		}

// 		new_position.y = std::min(new_position.y, wish_vertical_position);
// 	}

// 	if (!is_on_floor && was_on_floor && stick_to_floor && velocity.y > 0.02f) {
// 		bot = i32(new_position.y + size.y * 0.5f) + 1;

// 		// Move down until we hit a floor.
// 		for (i32 i = 0; i <= max_steps_height; i++) {
// 			if (is_row_blocked(
// 						left,
// 						right,
// 						bot)) {
// 				velocity.y = 0.0f;
// 				f32 pre_step_y = new_position.y;
// 				new_position.y = std::floor(new_position.y + 1.0f) - 0.02f;
// 				step_offset -= new_position.y - pre_step_y;
// 				is_on_floor = true;
// 				break;
// 			} else {
// 				bot += 1;
// 				new_position.y += 1.0f;
// 				step_offset -= 1.0f;
// 			}
// 		}

// 		if (!is_on_floor) {
// 			new_position.y -= f32(max_steps_height);
// 			step_offset += f32(max_steps_height);
// 		}
// 	}

// 	if (is_on_floor && stick_to_floor) {
// 		velocity.y = 0.0f;
// 	}

// 	new_position.y += step_offset;
// 	set_position(new_position);
// }

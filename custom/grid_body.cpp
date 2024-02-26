#include "grid_body.h"

#include "cell.hpp"
#include "cell_material.h"
#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/typedefs.h"
#include "grid.h"
#include "preludes.h"

const f32 SMALL_VALUE = 0.04f;

// Coords are relative to top left cell of top left chunk.
struct GridBodyApi {
	i32 max_step_height;

	// In global space.
	Vector2 true_pos;
	Vector2 true_velocity;
	bool is_on_floor;
	bool is_on_ceiling;
	bool is_on_left_wall;
	bool is_on_right_wall;

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

	inline GridBodyApi(Vector2 pos, Vector2 vel, f32 dt, Vector2 half_size, i32 p_max_step_height) :
			max_step_height(p_max_step_height),
			true_pos(pos),
			true_velocity(vel),
			is_on_floor(false),
			is_on_ceiling(false),
			is_on_left_wall(false),
			is_on_right_wall(false),
			wish_move(vel * dt) {
		Rect2 rect = Rect2(pos - half_size, half_size * 2.0f);
		rect = rect.merge(Rect2((pos + vel) - half_size, half_size * 2.0f));

		rect.grow_by(4.0f);
		rect = rect.grow_side(SIDE_TOP, Math::abs(vel.x) * f32(p_max_step_height));

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

	inline GridBodyApi(const GridBodyApi &other) :
			max_step_height(other.max_step_height),
			true_pos(other.true_pos),
			true_velocity(other.true_velocity),
			is_on_floor(other.is_on_floor),
			is_on_ceiling(other.is_on_ceiling),
			is_on_left_wall(other.is_on_left_wall),
			is_on_right_wall(other.is_on_right_wall),
			wish_move(other.wish_move),
			top(other.top),
			bot(other.bot),
			left(other.left),
			right(other.right),
			start(other.start),
			end(other.end),
			originf(other.originf),
			origini(other.origini),
			chunks_size(other.chunks_size),
			chunks(other.chunks) {
	}

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

		u32 cell = chunk->get_cell(Vector2i(coord.x & 31, coord.y & 31));
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
		// todo: could be faster
		for (i32 y = start; y < end; y++) {
			if (is_blocked(Vector2i(x, y), collision_bitmask)) {
				return end - y;
			}
		}

		return 0;
	}

	inline bool clamp_down() {
		start = i32(Math::floor(left));
		end = i32(Math::ceil(right));

		f32 new_bot = Math::ceil(bot) - SMALL_VALUE;
		f32 dif = new_bot - bot;

		if (dif > wish_move.y) {
			true_pos.y += wish_move.y;
			bot += wish_move.y;
			top += wish_move.y;

			wish_move.y = 0.0f;

			return false;
		} else {
			true_pos.y += dif;
			bot += dif;
			top += dif;

			wish_move.y -= dif;

			return true;
		}
	}

	inline bool step_down() {
		f32 dif = 1.0f;
		bool keep_going = true;
		if (dif > wish_move.y) {
			dif = wish_move.y;
			keep_going = false;
		}

		f32 new_bot = bot + dif;
		if (is_row_blocked(
					i32(Math::floor(new_bot)),
					CellCollision::CELL_COLLISION_SOLID)) {
			true_velocity.y = 0.0f;
			is_on_floor = true;
			return false;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot = new_bot;
		top += dif;

		return keep_going;
	}

	inline bool clamp_up() {
		start = i32(Math::floor(left));
		end = i32(Math::ceil(right));

		f32 new_top = Math::floor(top) + SMALL_VALUE;
		f32 dif = new_top - top;

		if (dif < wish_move.y) {
			true_pos.y += wish_move.y;
			bot += wish_move.y;
			top += wish_move.y;

			wish_move.y = 0.0f;

			return false;
		} else {
			true_pos.y += dif;
			bot += dif;
			top += dif;

			wish_move.y -= dif;

			return true;
		}
	}

	inline bool step_up() {
		f32 dif = -1.0f;
		bool keep_going = true;
		if (dif < wish_move.y) {
			dif = wish_move.y;
			keep_going = false;
		}

		f32 new_top = top + dif;
		if (is_row_blocked(
					i32(Math::floor(new_top)),
					CellCollision::CELL_COLLISION_SOLID)) {
			true_velocity.y = 0.0f;
			is_on_ceiling = true;
			return false;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot += dif;
		top = new_top;

		return keep_going;
	}

	inline bool clamp_right() {
		start = i32(Math::floor(top));
		end = i32(Math::ceil(bot));

		f32 new_right = Math::ceil(right) - SMALL_VALUE;
		f32 dif = new_right - right;

		if (dif > wish_move.x) {
			true_pos.x += wish_move.x;
			right += wish_move.x;
			left += wish_move.x;

			wish_move.x = 0.0f;

			return false;
		} else {
			true_pos.x += dif;
			right += dif;
			left += dif;

			wish_move.x -= dif;

			return true;
		}
	}

	inline bool step_right() {
		f32 dif = 1.0f;
		bool keep_going = true;
		if (dif > wish_move.x) {
			dif = wish_move.x;
			keep_going = false;
		}

		f32 new_right = right + dif;
		i32 wish_step = is_column_blocked(
				Math::floor(new_right),
				CellCollision::CELL_COLLISION_SOLID);
		if (wish_step != 0) {
			if (wish_step <= max_step_height) {
				GridBodyApi api = GridBodyApi(*this);
				f32 step_by = -Math::fract(bot) - f32(wish_step - 1) - SMALL_VALUE;
				api.wish_move.y = step_by;
				api.true_velocity.y = -1.0f;
				if (api.clamp_up()) {
					while (api.step_up()) {
					}
				}
				if (api.true_velocity.y != 0.0f) {
					// success
					f32 new_velocity = MIN(0.0f, true_velocity.y);
					*this = api;
					is_on_floor = true;
					true_velocity.y = new_velocity;
				} else {
					// faillure
					true_velocity.x = 0.0f;
					is_on_right_wall = true;
					return false;
				}
			} else {
				true_velocity.x = 0.0f;
				is_on_right_wall = true;
				return false;
			}
		}

		wish_move.x -= dif;
		true_pos.x += dif;
		right = new_right;
		left += dif;

		return keep_going;
	}

	inline bool clamp_left() {
		start = i32(Math::floor(top));
		end = i32(Math::ceil(bot));

		f32 new_left = Math::floor(left) + SMALL_VALUE;
		f32 dif = new_left - left;

		if (dif < wish_move.x) {
			true_pos.x += wish_move.x;
			right += wish_move.x;
			left += wish_move.x;

			wish_move.x = 0.0f;

			return false;
		} else {
			true_pos.x += dif;
			right += dif;
			left += dif;

			wish_move.x -= dif;

			return true;
		}
	}

	inline bool step_left() {
		f32 dif = -1.0f;
		bool keep_going = true;
		if (dif < wish_move.x) {
			dif = wish_move.x;
			keep_going = false;
		}

		f32 new_left = left + dif;
		i32 wish_step = is_column_blocked(
				Math::floor(new_left),
				CellCollision::CELL_COLLISION_SOLID);
		if (wish_step != 0) {
			if (wish_step <= max_step_height) {
				GridBodyApi api = GridBodyApi(*this);
				f32 step_by = -Math::fract(bot) - f32(wish_step - 1) - SMALL_VALUE;
				api.wish_move.y = step_by;
				api.true_velocity.y = -1.0f;
				if (api.clamp_up()) {
					while (api.step_up()) {
					}
				}
				if (api.true_velocity.y != 0.0f) {
					// success
					f32 new_velocity = MIN(0.0f, true_velocity.y);
					*this = api;
					is_on_floor = true;
					true_velocity.y = new_velocity;
				} else {
					// faillure
					true_velocity.x = 0.0f;
					is_on_left_wall = true;
					return false;
				}
			} else {
				true_velocity.x = 0.0f;
				is_on_left_wall = true;
				return false;
			}
		}

		wish_move.x -= dif;
		true_pos.x += dif;
		right += dif;
		left = new_left;

		return keep_going;
	}
};

void GridBody::_notification(i32 p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (Engine::get_singleton()->is_editor_hint() || draw_half_size) {
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
			D_METHOD("set_draw_half_size", "value"),
			&GridBody::set_draw_half_size);
	ClassDB::bind_method(
			D_METHOD("get_draw_half_size"),
			&GridBody::get_draw_half_size);
	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL,
					"draw_half_size"),
			"set_draw_half_size",
			"get_draw_half_size");

	ClassDB::bind_method(
			D_METHOD("set_stick_to_floor", "value"),
			&GridBody::set_stick_to_floor);
	ClassDB::bind_method(
			D_METHOD("get_stick_to_floor"),
			&GridBody::get_stick_to_floor);
	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL,
					"stick_to_floor"),
			"set_stick_to_floor",
			"get_stick_to_floor");

	ClassDB::bind_method(
			D_METHOD("set_collision", "value"),
			&GridBody::set_collision);
	ClassDB::bind_method(
			D_METHOD("get_collision"),
			&GridBody::get_collision);
	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL,
					"collision"),
			"set_collision",
			"get_collision");

	ClassDB::bind_method(
			D_METHOD("was_on_floor"),
			&GridBody::get_was_on_floor);
	ClassDB::bind_method(
			D_METHOD("is_on_floor"),
			&GridBody::get_is_on_floor);
	ClassDB::bind_method(
			D_METHOD("is_on_ceiling"),
			&GridBody::get_is_on_ceiling);
	ClassDB::bind_method(
			D_METHOD("is_on_left_wall"),
			&GridBody::get_is_on_left_wall);
	ClassDB::bind_method(
			D_METHOD("is_on_right_wall"),
			&GridBody::get_is_on_right_wall);
	ClassDB::bind_method(
			D_METHOD("is_on_wall"),
			&GridBody::get_is_on_wall);

	ClassDB::bind_method(
			D_METHOD("move_and_slide"),
			&GridBody::move_and_slide);

	ClassDB::bind_method(
			D_METHOD("get_floor_cell"),
			&GridBody::get_floor_cell);
}

void GridBody::set_half_size(Vector2 value) {
	half_size = value.abs();

	if (Engine::get_singleton()->is_editor_hint() || draw_half_size) {
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

void GridBody::set_draw_half_size(bool value) {
	draw_half_size = value;

	if (draw_half_size) {
		queue_redraw();
	}
}

bool GridBody::get_draw_half_size() const {
	return draw_half_size;
}

void GridBody::set_stick_to_floor(bool value) {
	stick_to_floor = value;
}

bool GridBody::get_stick_to_floor() const {
	return stick_to_floor;
}

void GridBody::set_collision(bool value) {
	collision = value;
}

bool GridBody::get_collision() const {
	return collision;
}

bool GridBody::get_was_on_floor() const {
	return was_on_floor;
}

bool GridBody::get_is_on_floor() const {
	return is_on_floor;
}

bool GridBody::get_is_on_ceiling() const {
	return is_on_ceiling;
}

bool GridBody::get_is_on_left_wall() const {
	return is_on_left_wall;
}

bool GridBody::get_is_on_right_wall() const {
	return is_on_right_wall;
}

bool GridBody::get_is_on_wall() const {
	return is_on_left_wall || is_on_right_wall;
}

void GridBody::move_and_slide() {
	was_on_floor = is_on_floor;
	is_on_floor = false;
	is_on_ceiling = false;
	is_on_left_wall = false;
	is_on_right_wall = false;

	step_offset *= 0.65f;

	if (!collision) {
		set_position(get_position() + velocity * get_process_delta_time());
		return;
	}

	GridBodyApi api = GridBodyApi(
			get_position(),
			velocity,
			get_process_delta_time(),
			half_size,
			i32(max_step_height));

	if (api.wish_move.y > SMALL_VALUE) {
		if (api.clamp_down()) {
			while (api.step_down()) {
			}
		}
	} else if (api.wish_move.y < -SMALL_VALUE) {
		if (api.clamp_up()) {
			while (api.step_up()) {
			}
		}
	}

	if (api.wish_move.x > SMALL_VALUE) {
		if (api.clamp_right()) {
			while (api.step_right()) {
			}
		}
	} else if (api.wish_move.x < -SMALL_VALUE) {
		if (api.clamp_left()) {
			while (api.step_left()) {
			}
		}
	}

	// Stick to floor.
	if (stick_to_floor && velocity.y >= -SMALL_VALUE && !api.is_on_floor) {
		GridBodyApi api2 = GridBodyApi(api);
		api2.wish_move.y = INF_F32;

		api2.clamp_down();
		for (i32 i = 0; i <= max_step_height - 1; i++) {
			if (!api2.step_down()) {
				break;
			}
		}

		if (api2.is_on_floor) {
			api = api2;
			api.true_velocity.y = 0.0f;
		}
	}

	is_on_floor = api.is_on_floor;
	is_on_ceiling = api.is_on_ceiling;
	is_on_left_wall = api.is_on_left_wall;
	is_on_right_wall = api.is_on_right_wall;
	set_velocity(api.true_velocity);
	set_position(api.true_pos);

	api.del();
}

u32 GridBody::get_floor_cell() const {
	if (is_on_floor) {
		Vector2 pos = get_position();
		pos.y += half_size.y + 1.0f;

		Vector2i coord = Vector2i(pos.floor());

		return Grid::get_cell_material_idx(coord);
	} else {
		return 0;
	}
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

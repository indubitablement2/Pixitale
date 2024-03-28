#include "grid_body.h"

#include "cell.hpp"
#include "cell_material.hpp"
#include "core/error/error_macros.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/typedefs.h"
#include "grid.h"
#include "preludes.h"
#include "scene/2d/node_2d.h"

const f32 SMALL_VALUE = 0.04f;

// Coords are relative to top left cell of top left chunk.
struct GridBodyApi {
	// In global space.
	Vector2 true_pos;

	// bool is_on_floor;
	// bool is_on_ceiling;
	// bool is_on_left_wall;
	// bool is_on_right_wall;

	Vector2 wish_move;

	f32 top;
	f32 bot;
	f32 left;
	f32 right;

	Vector2i chunks_size;
	Chunk **chunks;

	inline GridBodyApi(Vector2 pos, Vector2 vel, f32 dt, Vector2 half_size, i32 p_max_step_height) :
			true_pos(pos),
			wish_move(vel * dt) {
		Rect2 rect = Rect2(pos - half_size, half_size * 2.0f);
		rect = rect.merge(Rect2((pos + wish_move) - half_size, half_size * 2.0f));

		rect.grow_by(8.0f);
		f32 grow = (Math::abs(wish_move.x) + 1.0f) * f32(p_max_step_height);
		rect = rect.grow_individual(0.0f, grow, 0.0f, grow);

		Vector2i chunks_start = div_floor(Vector2i(rect.position.floor()), 32);
		Vector2i chunks_end = div_floor(Vector2i((rect.position + rect.size).floor()), 32) + Vector2i(1, 1);

		chunks_size = chunks_end - chunks_start;
		chunks = new Chunk *[chunks_size.x * chunks_size.y];

		for (i32 y = 0; y < chunks_size.y; y++) {
			for (i32 x = 0; x < chunks_size.x; x++) {
				chunks[y * chunks_size.x + x] = Grid::get_chunk(chunks_start + Vector2i(x, y));
			}
		}

		Vector2 origin = Vector2(chunks_start * 32);
		top = pos.y - half_size.y - origin.y;
		bot = pos.y + half_size.y - origin.y;
		left = pos.x - half_size.x - origin.x;
		right = pos.x + half_size.x - origin.x;
	}

	inline GridBodyApi(const GridBodyApi &other) :
			true_pos(other.true_pos),
			wish_move(other.wish_move),
			top(other.top),
			bot(other.bot),
			left(other.left),
			right(other.right),
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

		if (Cell::movement(cell) != -2) {
			// Moving cell.
			return false;
		}

		u32 mat_idx = Cell::material_idx(cell);

		if (mat_idx == 0) {
			// Empty cell.
			return false;
		}

		const CellMaterial &mat = Grid::get_cell_material(mat_idx);
		return (u32(mat.collision) & collision_bitmask) != 0;
	}

	inline bool is_row_blocked(i32 y, i32 start, i32 end, u32 collision_bitmask) {
		// todo: could be faster
		for (i32 x = start; x < end; x++) {
			if (is_blocked(Vector2i(x, y), collision_bitmask)) {
				return true;
			}
		}

		return false;
	}

	// Return the number of cells from end. Eg. the step height.
	inline i32 is_column_blocked(i32 x, i32 start, i32 end, u32 collision_bitmask) {
		// todo: could be faster
		for (i32 y = start; y < end; y++) {
			if (is_blocked(Vector2i(x, y), collision_bitmask)) {
				return end - y;
			}
		}

		return 0;
	}

	inline void clamp_down() {
		f32 dif = (Math::ceil(bot) - SMALL_VALUE) - bot;
		dif = MIN(dif, wish_move.y);

		true_pos.y += dif;
		bot += dif;
		top += dif;

		wish_move.y -= dif;
	}

	// True if blocked and couldn't move.
	inline bool step_down() {
		f32 dif = MIN(1.0f, wish_move.y);

		f32 new_bot = bot + dif;
		if (is_row_blocked(
					i32(Math::floor(new_bot)),
					i32(Math::floor(left)),
					i32(Math::ceil(right)),
					CellCollision::CELL_COLLISION_SOLID)) {
			return true;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot = new_bot;
		top += dif;

		return false;
	}

	inline void clamp_up() {
		f32 dif = (Math::floor(top) + SMALL_VALUE) - top;
		dif = MAX(dif, wish_move.y);

		true_pos.y += dif;
		bot += dif;
		top += dif;

		wish_move.y -= dif;
	}

	// True if blocked and couldn't move.
	inline bool step_up() {
		f32 dif = MAX(-1.0f, wish_move.y);

		f32 new_top = top + dif;
		if (is_row_blocked(
					i32(Math::floor(new_top)),
					i32(Math::floor(left)),
					i32(Math::ceil(right)),
					CellCollision::CELL_COLLISION_SOLID)) {
			return true;
		}

		wish_move.y -= dif;
		true_pos.y += dif;
		bot += dif;
		top = new_top;

		return false;
	}

	inline void clamp_right() {
		f32 dif = (Math::ceil(right) - SMALL_VALUE) - right;
		dif = MIN(dif, wish_move.x);

		true_pos.x += dif;
		right += dif;
		left += dif;

		wish_move.x -= dif;
	}

	inline i32 step_right() {
		f32 dif = MIN(1.0f, wish_move.x);

		f32 new_right = right + dif;
		i32 wish_step = is_column_blocked(
				Math::floor(new_right),
				i32(Math::floor(top)),
				i32(Math::ceil(bot)),
				CellCollision::CELL_COLLISION_SOLID);
		if (wish_step == 0) {
			wish_move.x -= dif;
			true_pos.x += dif;
			right = new_right;
			left += dif;
		}
		return wish_step;
	}

	inline void clamp_left() {
		f32 dif = (Math::floor(left) + SMALL_VALUE) - left;
		dif = MAX(dif, wish_move.x);

		true_pos.x += dif;
		right += dif;
		left += dif;

		wish_move.x -= dif;
	}

	inline i32 step_left() {
		f32 dif = MAX(-1.0f, wish_move.x);

		f32 new_left = left + dif;
		i32 wish_step = is_column_blocked(
				Math::floor(new_left),
				i32(Math::floor(top)),
				i32(Math::ceil(bot)),
				CellCollision::CELL_COLLISION_SOLID);
		if (wish_step == 0) {
			wish_move.x -= dif;
			true_pos.x += dif;
			right += dif;
			left = new_left;
		}
		return wish_step;
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
			D_METHOD("set_step_smoothing", "value"),
			&GridBody::set_step_smoothing);
	ClassDB::bind_method(
			D_METHOD("get_step_smoothing"),
			&GridBody::get_step_smoothing);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT,
					"step_smoothing", PROPERTY_HINT_RANGE, "0.0,0.99,0.01"),
			"set_step_smoothing",
			"get_step_smoothing");

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
			D_METHOD("set_collision_enabled", "value"),
			&GridBody::set_collision_enabled);
	ClassDB::bind_method(
			D_METHOD("get_collision_enabled"),
			&GridBody::get_collision_enabled);
	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL,
					"collision"),
			"set_collision_enabled",
			"get_collision_enabled");

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

void GridBody::set_step_smoothing(f32 value) {
	step_smoothing = value;
}

f32 GridBody::get_step_smoothing() const {
	return step_smoothing;
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

void GridBody::set_collision_enabled(bool value) {
	collision_enabled = value;
}

bool GridBody::get_collision_enabled() const {
	return collision_enabled;
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

	Node2D *p = Object::cast_to<Node2D>(get_parent());
	ERR_FAIL_NULL_MSG(p, "GridBody must be a child of a Node2D.");

	if (!collision_enabled) {
		f32 dif = step_offset;
		step_offset *= step_smoothing;
		dif -= step_offset;
		p->translate(velocity * get_process_delta_time() + Vector2(0.0f, dif));
		return;
	}

	Vector2 previous_position = get_global_position();
	previous_position.y += step_offset;

	GridBodyApi api = GridBodyApi(
			previous_position,
			velocity,
			get_process_delta_time(),
			half_size,
			max_step_height);

	// Move left or right.
	i32 num_vertical_steps = 0;
	if (api.wish_move.x > 0.0f) {
		api.clamp_right();

		while (api.wish_move.x > 0.0f) {
			i32 wish_step = api.step_right();

			if (wish_step == 0) {
				num_vertical_steps += 1;
			} else if (wish_step <= max_step_height) {
				GridBodyApi api2 = GridBodyApi(api);
				f32 step_by = -Math::fract(api.bot) - f32(wish_step - 1) - SMALL_VALUE;
				api2.wish_move.y = step_by;
				api2.clamp_up();
				bool success = true;

				while (api2.wish_move.y < -0.0f) {
					if (api2.step_up()) {
						success = false;
						break;
					}
				}

				if (success && api2.step_right() == 0) {
					num_vertical_steps += 1;
					step_offset += step_by;
					api = api2;
				} else {
					velocity.x = 0.0f;
					is_on_right_wall = true;
					break;
				}
			} else {
				velocity.x = 0.0f;
				is_on_right_wall = true;
				break;
			}
		}

	} else if (api.wish_move.x < -0.0f) {
		api.clamp_left();

		while (api.wish_move.x < -0.0f) {
			i32 wish_step = api.step_left();

			if (wish_step == 0) {
				num_vertical_steps += 1;
			} else if (wish_step <= max_step_height) {
				GridBodyApi api2 = GridBodyApi(api);
				f32 step_by = -Math::fract(api.bot) - f32(wish_step - 1) - SMALL_VALUE;
				api2.wish_move.y = step_by;
				api2.clamp_up();
				bool success = true;

				while (api2.wish_move.y < -0.0f) {
					if (api2.step_up()) {
						success = false;
						break;
					}
				}

				if (success && api2.step_left() == 0) {
					num_vertical_steps += 1;
					step_offset += step_by;
					api = api2;
				} else {
					velocity.x = 0.0f;
					is_on_left_wall = true;
					break;
				}
			} else {
				velocity.x = 0.0f;
				is_on_left_wall = true;
				break;
			}
		}
	}

	// Move up or down.
	if (api.wish_move.y > 0.0f) {
		api.clamp_down();
		while (api.wish_move.y > 0.0) {
			if (api.step_down()) {
				velocity.y = 0.0f;
				is_on_floor = true;
				break;
			}
		}
	} else if (api.wish_move.y < -0.0) {
		api.clamp_up();

		while (api.wish_move.y < -0.0) {
			if (api.step_up()) {
				velocity.y = 0.0f;
				is_on_ceiling = true;
				break;
			}
		}
	}

	// Stick to floor.
	if (stick_to_floor && velocity.y > -SMALL_VALUE && !is_on_floor) {
		GridBodyApi api2 = GridBodyApi(api);
		api2.wish_move.y = INF_F32;

		api2.clamp_down();
		bool success = false;
		// * (num_vertical_steps / 2 + 1)
		for (i32 i = 0; i < max_step_height * (num_vertical_steps / 2 + 1); i++) {
			if (api2.step_down()) {
				success = true;
				break;
			}
		}

		if (success) {
			step_offset += api2.true_pos.y - api.true_pos.y;
			api = api2;
			velocity.y = 0.0f;
			is_on_floor = true;
		}
	}

	step_offset *= step_smoothing;

	Vector2 new_position = api.true_pos;
	new_position.y -= step_offset;
	p->translate(new_position - get_global_position());

	// p->set_position(Vector2(api.true_pos.x, api.true_pos.y - step_offset));

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

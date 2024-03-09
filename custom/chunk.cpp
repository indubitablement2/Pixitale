#include "chunk.h"
#include "cell.hpp"
#include "cell_material.hpp"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "grid.h"
#include "preludes.h"
#include <bit>

// Api for working with cells within a single chunk (center)
// which may affect nearby chunks.
// Coord is relative to top left cell of top left chunk unless otherwise specified.
class ChunkApi {
public:
	u32 *cell_ptr;
	u32 cell;

	u32 cell_material_idx;
	const CellMaterial *cell_material;

	// Coord of the current cell relative to cell_coord_origin.
	Vector2i cell_coord;

	// Coord of the top left cell of top left chunk.
	Vector2i cell_coord_origin;

	Rng rng;

	std::vector<std::pair<Callable *, Vector2i>> &reaction_callbacks;

	Chunk *chunks[9];

	ChunkApi(Vector2i chunk_coord) :
			cell_coord_origin((chunk_coord - Vector2i(1, 1)) * 32),
			rng(Grid::get_temporal_rng(chunk_coord)),
			reaction_callbacks(Grid::get_reaction_callback_vector()) {
	}

	Chunk *center() {
		return chunks[4];
	}

	void bound_test(Vector2i coord) {
		TEST_ASSERT(coord.x >= 0, "coord.x is too small");
		TEST_ASSERT(coord.y >= 0, "coord.y is too small");
		TEST_ASSERT(coord.x < 96, "coord.x is too large");
		TEST_ASSERT(coord.y < 96, "coord.y is too large");
	}

	// Returns chunk idx and transform coord to be local to this chunk.
	i32 to_local(Vector2i &coord) {
		bound_test(coord);

		i32 chunk_x = coord.x >> 5;
		i32 chunk_y = coord.y >> 5;

		coord.x &= 31;
		coord.y &= 31;

		return chunk_x + chunk_y * 3;
	}

	inline u32 *get_ptr(Vector2i coord) {
		i32 chunk_idx = to_local(coord);
		return chunks[chunk_idx]->get_cell_ptr(coord);
	}

	void activate_point(Vector2i coord, bool activate_cell) {
		i32 chunk_idx = to_local(coord);
		chunks[chunk_idx]->activate_point(coord, activate_cell);
	}

	// Does not activate the cell at coord itself.
	void activate_neightbors(Vector2i coord) {
		activate_point(coord + Vector2i(-1, -1), true);
		activate_point(coord + Vector2i(0, -1), true);
		activate_point(coord + Vector2i(1, -1), true);

		activate_point(coord + Vector2i(-1, 0), true);
		// center point is activated as a byproduct.
		activate_point(coord + Vector2i(1, 0), true);

		activate_point(coord + Vector2i(-1, 1), true);
		activate_point(coord + Vector2i(0, 1), true);
		activate_point(coord + Vector2i(1, 1), true);
	}

	bool is_row_active(Vector2i coord) {
		i32 chunk_idx = to_local(coord);
		return chunks[chunk_idx]->is_row_active(coord.y);
	}

	void try_react_between(Vector2i dir) {
		Vector2i other_coord = cell_coord + dir;
		u32 *other_cell_ptr = get_ptr(other_coord);
		u32 other = *other_cell_ptr;
		u32 other_material_idx = Cell::material_idx(other);

		bool swap;
		CellReaction *reaction = nullptr;
		CellReaction *reaction_end = nullptr;
		Grid::reactions_between(
				reaction,
				reaction_end,
				cell_material_idx,
				other_material_idx,
				swap);

		if (reaction == nullptr) {
			return;
		}

		// We have at least one possible reaction.
		Cell::set_active(cell);

		while (reaction != reaction_end) {
			if (reaction->try_react(rng)) {
				if (!reaction->callback.is_null()) {
					// Make sure trigger coord is at `in1`.
					Vector2i trigger_coord = cell_coord_origin;
					// if (swap && reaction->callback_swap) {
					// 	trigger_coord += cell_coord;
					// } else if (!swap && reaction->callback_swap) {
					// 	trigger_coord += other_coord;
					// } else if (swap && !reaction->callback_swap) {
					// 	trigger_coord += other_coord;
					// } else {
					// 	trigger_coord += cell_coord;
					// }
					// Simplified version of the above.
					if (swap == reaction->callback_swap) {
						trigger_coord += cell_coord;
					} else {
						trigger_coord += other_coord;
					}
					reaction_callbacks.push_back({ &reaction->callback, trigger_coord });
				}

				u32 cell_material_idx_out;
				u32 other_material_idx_out;
				if (swap) {
					cell_material_idx_out = reaction->mat_idx_out2;
					other_material_idx_out = reaction->mat_idx_out1;
				} else {
					cell_material_idx_out = reaction->mat_idx_out1;
					other_material_idx_out = reaction->mat_idx_out2;
				}

				if (cell_material_idx != cell_material_idx_out) {
					cell_material_idx = cell_material_idx_out;
					cell_material = &Grid::get_cell_material(cell_material_idx);

					cell = 0;
					Cell::set_material_idx(cell, cell_material_idx);
					Cell::set_active(cell);

					if (cell_material->new_cell_noise_max > 0) {
						Cell::set_color_value(cell, rng.gen_color_value(cell_material->new_cell_noise_max));
					}

					activate_neightbors(cell_coord);
				}

				if (other_material_idx != other_material_idx_out) {
					other = 0;
					Cell::set_material_idx(other, other_material_idx_out);
					Cell::set_active(other);

					const CellMaterial &other_material = Grid::get_cell_material(other_material_idx_out);
					if (other_material.new_cell_noise_max > 0) {
						Cell::set_color_value(cell, rng.gen_color_value(other_material.new_cell_noise_max));
					}

					*other_cell_ptr = other;

					activate_neightbors(other_coord);
				}

				break;
			}

			reaction += 1;
		}
	}

	// Returns true if swapped.
	bool try_move(Vector2i dir) {
		Vector2i other_coord = cell_coord + dir;
		u32 *other_cell_ptr = get_ptr(other_coord);
		u32 other = *other_cell_ptr;
		u32 other_material_idx = Cell::material_idx(other);

		if (other_material_idx == cell_material_idx) {
			// Quick path for same material.
			return false;
		}

		const CellMaterial &other_material = Grid::get_cell_material(other_material_idx);

		if (cell_material->density > other_material.density) {
			*cell_ptr = other;
			cell_ptr = other_cell_ptr;

			activate_neightbors(other_coord);
			activate_neightbors(cell_coord);
			cell_coord = other_coord;

			Cell::set_active(cell);

			return true;
		} else {
			return false;
		}
	}

	void step_cell(const Vector2i center_coord, const bool force_step) {
		cell_ptr = center()->get_cell_ptr(center_coord);
		cell = *cell_ptr;
		cell_coord = center_coord + Vector2i(32, 32);

		bool was_active = Cell::is_active(cell);

		if (Cell::is_updated(cell, Grid::cell_updated_bitmask)) {
			if (was_active) {
				center()->activate_point(center_coord, false);
			}
			return;
		}

		if (!force_step && !was_active) {
			return;
		}

		Cell::set_active(cell, false);

		cell_material_idx = Cell::material_idx(cell);
		cell_material = &Grid::get_cell_material(cell_material_idx);

		// Reactions
		// We react with `o`s, `x`s will react with us.
		// x x x
		// o @ x
		// o o o
		try_react_between(Vector2i(-1, 0));
		try_react_between(Vector2i(-1, 1));
		try_react_between(Vector2i(0, 1));
		try_react_between(Vector2i(1, 1));

		// // Particle movement
		// if (Cell::movement(cell)) {
		// 	velocity = *velocity_ptr;

		// 	velocity.y += cell_material.gravity;
		// 	velocity *= cell_material.friction;
		// 	velocity = Vector2(
		// 			CLAMP(velocity.x, -16.0f, 16.0f),
		// 			CLAMP(velocity.y, -16.0f, 16.0f));

		// 	Vector2i target = Vector2i(velocity);
		// 	// Fractional part of velocity is handled using probability.
		// 	Vector2 frac = velocity - Vector2(target);
		// 	if (rng.gen_probability_f32(frac.x)) {
		// 		target.x += i32(SIGN(velocity.x));
		// 	}
		// 	if (rng.gen_probability_f32(frac.y)) {
		// 		target.y += i32(SIGN(velocity.y));
		// 	}

		// 	if (target != Vector2i(0, 0)) {
		// 		IterLine iter = IterLine(target, false);
		// 		Vector2i new_coord = cell_coord;
		// 		while (iter.next()) {
		// 			Vector2i other_coord = iter.currenti() + cell_coord;

		// 			u32 *other_cell_ptr;
		// 			get_ptr(other_coord, &other_cell_ptr);
		// 			u32 other = *other_cell_ptr;

		// 			u32 other_material_idx = Cell::material_idx(other);
		// 			if (other_material_idx == cell_material_idx) {
		// 				// todo: Can pass through same material cells.
		// 				continue;
		// 			}

		// 			CellMaterial &other_cell_material = Grid::get_cell_material(other_material_idx);
		// 			if (cell_material.density > other_cell_material.density) {
		// 				// Can swap with less dense cells.
		// 				new_coord = other_coord;
		// 				continue;
		// 			}

		// 			if (Cell::movement(other)) {
		// 				// Can pass through particle cells.
		// 				continue;
		// 			}

		// 			// Blocked.
		// 			velocity = -Vector2(
		// 					velocity.x * rng.gen_f32() * cell_material.bounciness,
		// 					velocity.y * rng.gen_f32() * cell_material.bounciness);
		// 			break;
		// 		}

		// 		if (new_coord != cell_coord) {
		// 			swap(new_coord);
		// 		}
		// 	}
		// } else {
		// 	velocity = Vector2(0.0f, 0.0f);
		// }

		// Vertical movement
		i32 movement = Cell::movement(cell);
		i32 vertical_dir = SIGN(cell_material->vertical_movement);
		for (i32 i = 0; i != cell_material->vertical_movement; i += vertical_dir) {
			if (try_move(Vector2i(0, vertical_dir))) {
				movement = 0;
			} else {
				if (movement == 0) {
					// Always move horizontally after falling.
					movement = rng.gen_sign();
				}

				break;
			}
		}

		// Spontaneously start moving.
		if (was_active && movement == -2 && rng.gen_probability_u32_max(cell_material->horizontal_movement_start_chance)) {
			movement = rng.gen_sign();
		}

		// Horizontal movement
		if (movement == -1 || movement == 1) {
			if (rng.gen_probability_u32_max(cell_material->horizontal_movement_stop_chance)) {
				// Spontaneously stop moving.
				movement = -2;
			} else {
				for (i32 i = 0; i < cell_material->horizontal_movement; i++) {
					// Try move diagonally first.
					if (try_move(Vector2i(movement, vertical_dir))) {
						continue;
					}
					// Then horizontally.
					if (try_move(Vector2i(movement, 0))) {
						continue;
					}

					if (rng.gen_probability_u32_max(cell_material->dissipate_on_horizontal_blocked_chance)) {
						if (!is_row_active(Vector2i(cell_coord.x, cell_coord.y + 2))) {
							// Dissipate on blocked horizontal movement.
							cell = 0;
							Cell::set_active(cell, true);
							break;
						}
					}

					if (cell_material->can_reverse_horizontal_movement) {
						// Try other direction.
						movement = -movement;

						if (try_move(Vector2i(movement, vertical_dir))) {
						} else if (try_move(Vector2i(movement, 0))) {
						} else {
							// Blocked on both sides.
							movement = -2;
						}
					} else {
						// Blocked on moving side and doesn't want to reverse.
						movement = -2;
					}
					break;
				}
			}
		}

		Cell::set_movement(cell, movement);
		if (movement != -2) {
			Cell::set_active(cell, true);
		}

		if (Cell::is_active(cell)) {
			Cell::set_updated(cell, Grid::cell_updated_bitmask);
			// This is for the case where we didn't move/react, but are active nonetheless.
			// Otherwise this does nothing as we would call activate_neightbors.
			center()->activate_point(center_coord, false);
		} else {
			Cell::clear_updated(cell);
		}

		*cell_ptr = cell;
	}
};

void Chunk::step_chunk(Vector2i chunk_coord) {
	ChunkApi chunk_api = ChunkApi(chunk_coord);

	for (i32 y = -1; y <= 1; y++) {
		for (i32 x = -1; x <= 1; x++) {
			Vector2i other_chunk_coord = chunk_coord + Vector2i(x, y);
			Chunk *chunk_ptr = Grid::get_chunk(other_chunk_coord);

			TEST_ASSERT(chunk_ptr != nullptr, "step chunk got nullptr");

			// Chunk generation
			if (chunk_ptr->last_step_tick < 0) {
				chunk_ptr->last_step_tick = 0;

				// Clear cells.
				for (i32 i = 0; i < 32 * 32; i++) {
					chunk_ptr->cells[i] = 0;
				}

				Grid::generate_chunk(other_chunk_coord);
			}

			chunk_api.chunks[(x + 1) + (y + 1) * 3] = chunk_ptr;
		}
	}

	bool force_step = chunk_api.center()->last_step_tick <= Grid::last_modified_tick;
	chunk_api.center()->last_step_tick = Grid::get_tick();

	if (chunk_api.center()->is_inactive() && !force_step) {
		// Nothing to do.
		return;
	}

	// i32 y_start = std::countr_zero(active_rows);
	// i32 y_end = 32 - std::countl_zero(active_rows);

	i32 y_top;
	i32 y_bot;
	i32 x_start;
	i32 x_end;
	i32 x_step;
	u32 active_rows;
	if (force_step) {
		y_top = -1;
		y_bot = 31;
		x_start = 0;
		x_end = 32;
		x_step = 1;
		active_rows = MAX_U32;
	} else {
		active_rows = chunk_api.center()->active_rows;
		u32 active_columns = chunk_api.center()->active_columns;

		y_top = std::countr_zero(active_rows) - 1;
		y_bot = 31 - std::countl_zero(active_rows);
		TEST_ASSERT(y_top < y_bot, "y_top is >= than y_top");

		i32 x_start_base = std::countr_zero(active_columns);
		i32 x_end_base = 32 - std::countl_zero(active_columns);
		// Alternate iteration between left and right.
		// Reduces visible chunk border artifacts.
		if ((Grid::get_tick() & 1) == 0) {
			x_step = -1;
			x_start = x_end_base - 1;
			x_end = x_start_base - 1;
		} else {
			x_step = 1;
			x_start = x_start_base;
			x_end = x_end_base;
		}
	}

	chunk_api.center()->clear_active_rect();

	// Iterate over each cell in the chunk from the bottom.
	for (i32 y = y_bot; y != y_top; y--) {
		if ((active_rows & (1u << y)) == 0) {
			continue;
		}

		i32 x = x_start;
		while (x != x_end) {
			chunk_api.step_cell(Vector2i(x, y), force_step);
			x += x_step;
		}
	}
}
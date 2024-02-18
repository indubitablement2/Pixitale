#include "chunk.h"
#include "cell.hpp"
#include "cell_material.h"
#include "core/math/math_funcs.h"
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
	u32 cell;
	u32 *cell_ptr;
	Vector2i cell_coord;
	Vector2i cell_coord_origin;
	Rng rng;
	std::vector<std::pair<Callable *, Vector2i>> &reaction_callbacks = Grid::get_reaction_callback_vector();
	Chunk *chunks[9];

	ChunkApi(Vector2i chunk_coord) :
			cell_coord_origin(chunk_coord * 32),
			rng(Grid::get_temporal_rng(chunk_coord)) {
	}

	Chunk *center() {
		return chunks[4];
	}

	u32 cell_material_idx() {
		return Cell::material_idx(cell);
	}

	void set_cell_material_idx(u32 new_cell_material_idx) {
		Cell::set_material_idx(cell, new_cell_material_idx);
	}

	CellMaterial &cell_material() {
		return Grid::get_cell_material(cell_material_idx());
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

	void get_ptrs(Vector2i coord, Chunk *&chunk_ptr, u32 *&other_cell_ptr) {
		i32 chunk_idx = to_local(coord);
		chunk_ptr = chunks[chunk_idx];
		other_cell_ptr = chunk_ptr->get_cell_ptr(coord);
	}

	u32 *get_cell_ptr(Vector2i coord) {
		i32 chunk_idx = to_local(coord);
		return chunks[chunk_idx]->get_cell_ptr(coord);
	}

	u32 get_cell(Vector2i coord) {
		return *get_cell_ptr(coord);
	}

	// void set_cell(Vector2i coord, u32 cell) {
	// 	*get_cell_ptr(coord) = cell;
	// }

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
		// center chunk is activated as a byproduct.
		activate_point(coord + Vector2i(1, 0), true);

		activate_point(coord + Vector2i(-1, 1), true);
		activate_point(coord + Vector2i(0, 1), true);
		activate_point(coord + Vector2i(1, 1), true);
	}

	bool is_row_active(Vector2i coord) {
		i32 chunk_idx = to_local(coord);
		return chunks[chunk_idx]->is_row_active(coord.y);
	}

	bool can_swap(Vector2i other_coord) {
		u32 other = get_cell(other_coord);
		CellMaterial &other_cell_material = Grid::get_cell_material(Cell::material_idx(other));
		return cell_material().density > other_cell_material.density;
	}

	// Returns true if swapped.
	bool try_swap(Vector2i other_coord) {
		Chunk *other_chunk_ptr;
		u32 *other_cell_ptr;
		get_ptrs(other_coord, other_chunk_ptr, other_cell_ptr);
		u32 other = *other_cell_ptr;
		CellMaterial &other_cell_material = Grid::get_cell_material(Cell::material_idx(other));

		if (cell_material().density > other_cell_material.density) {
			// todo: duplication on h movement

			Cell::set_updated(other, Grid::cell_updated_bitmask);
			Cell::set_updated(cell, Grid::cell_updated_bitmask);

			*cell_ptr = other;
			cell_ptr = other_cell_ptr;

			activate_neightbors(other_coord);
			activate_neightbors(cell_coord);
			cell_coord = other_coord;

			return true;
		} else {
			return false;
		}
	}

	void try_react_between(Vector2i other_offset) {
		Vector2i other_coord = cell_coord + other_offset;
		Chunk *other_chunk_ptr;
		u32 *other_cell_ptr;
		get_ptrs(other_coord, other_chunk_ptr, other_cell_ptr);
		u32 other = *other_cell_ptr;
		u32 other_material_idx = Cell::material_idx(other);

		bool swap;
		CellReaction *reaction = nullptr;
		CellReaction *reaction_end = nullptr;
		Grid::reactions_between(reaction, reaction_end, cell_material_idx(), other_material_idx, swap);

		if (reaction == nullptr) {
			return;
		}

		Cell::set_active(cell);

		while (reaction != reaction_end) {
			if (reaction->try_react(rng)) {
				u32 cell_material_idx_out;
				u32 other_material_idx_out;
				if (swap) {
					cell_material_idx_out = reaction->mat_idx_out2;
					other_material_idx_out = reaction->mat_idx_out1;
				} else {
					cell_material_idx_out = reaction->mat_idx_out1;
					other_material_idx_out = reaction->mat_idx_out2;
				}

				if (cell_material_idx() != cell_material_idx_out) {
					// todo: color
					cell = Cell::build_cell(
							cell_material_idx_out,
							Grid::cell_updated_bitmask,
							true);
					activate_neightbors(cell_coord);
				}

				if (other_material_idx != other_material_idx_out) {
					// todo: color
					*other_cell_ptr = Cell::build_cell(
							other_material_idx_out,
							Grid::cell_updated_bitmask,
							true);
					activate_neightbors(other_coord);
				}

				if (!reaction->callback.is_null()) {
					reaction_callbacks.push_back({ &reaction->callback, cell_coord + cell_coord_origin });
				}

				break;
			}

			reaction += 1;
		}
	}

	void step_cell(const Vector2i center_coord, bool force_step) {
		cell_ptr = center()->get_cell_ptr(center_coord);
		cell = *cell_ptr;
		cell_coord = center_coord + Vector2i(32, 32);

		if (force_step) {
			if (Cell::is_updated(cell, Grid::cell_updated_bitmask)) {
				if (Cell::is_active(cell)) {
					center()->activate_point(center_coord, false);
				}
				return;
			}
		} else {
			if (!Cell::is_active(cell)) {
				return;
			}

			if (Cell::is_updated(cell, Grid::cell_updated_bitmask)) {
				center()->activate_point(center_coord, false);
				return;
			}
		}

		Cell::set_updated(cell, Grid::cell_updated_bitmask);
		Cell::set_active(cell, false);

		// Reactions
		// We react with `o`s, `x`s will react with us.
		// x x x
		// o @ x
		// o o o
		try_react_between(Vector2i(-1, 0));
		try_react_between(Vector2i(-1, 1));
		try_react_between(Vector2i(0, 1));
		try_react_between(Vector2i(1, 1));

		if (Cell::is_active(cell)) {
			center()->activate_point(center_coord, false);
		}

		// Movement
		if (Cell::is_particle(cell)) {
			// todo
		} else {
			u32 orthogonal_velocity_i = Cell::orthogonal_velocity_i(cell);
			if (!cell_material().can_fall && orthogonal_velocity_i == 0) {
				Cell::clear_velocity(cell);
				*cell_ptr = cell;
				return;
			}
			f32 orthogonal_velocity = f32(orthogonal_velocity_i) / 255.0f;

			i32 v_dir;
			f32 vertical_acceleration = cell_material().vertical_acceleration;
			if (vertical_acceleration >= 0) {
				v_dir = 1;
			} else {
				v_dir = -1;
				vertical_acceleration = -vertical_acceleration;
			}

			if (can_swap(Vector2i(cell_coord.x, cell_coord.y + v_dir))) {
				// Vertical movement

				if (Cell::movement_dir(cell) != 0) {
					// We were moving horizontally before.
					Cell::set_movement_dir(cell, 0);
					orthogonal_velocity = 0.0f;
				}

				orthogonal_velocity += vertical_acceleration;
				orthogonal_velocity = MIN(orthogonal_velocity, 1.0);

				f32 vel = orthogonal_velocity * cell_material().vertical_velocity_max;
				i32 v_num = i32(vel) + i32(rng.gen_probability_f32(vel - Math::floor(vel)));
				for (i32 i = 0; i < v_num; i++) {
					if (!try_swap(Vector2i(cell_coord.x, cell_coord.y + v_dir))) {
						// Blocked. Transfer some vertical velocity to horizontal.
						orthogonal_velocity *= cell_material().vertical_velocity_max / cell_material().horizontal_velocity_max;
						orthogonal_velocity *= 0.75f;
						Cell::set_movement_dir(cell, rng.gen_sign());
						break;
					}
				}

				Cell::set_orthogonal_velocity(cell, orthogonal_velocity);
			} else {
				// Horizontal movement

				u32 h_dir = Cell::movement_dir(cell);
				if (h_dir == 0) {
					// We were moving vertically before.
					h_dir = rng.gen_sign();
				}

				// todo before each h move, check if can move diag first

				Cell::set_movement_dir(cell, h_dir);
				Cell::set_orthogonal_velocity(cell, 0.0f);
			}
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
			if (!chunk_ptr->generated) {
				chunk_ptr->generated = true;

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

	chunk_api.rng = Grid::get_temporal_rng(chunk_coord);

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
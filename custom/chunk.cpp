#include "chunk.h"
#include "cell.hpp"
#include "cell_material.h"
#include "cell_reaction.h"
#include "core/math/math_funcs.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "grid.h"
#include <bit>

// Api for working with cells within a single chunk (center)
// which may affect nearby chunks.
// Coord is relative to top left cell of top left chunk unless otherwise specified.
class ChunkApi {
public:
	u32 current_cell_updated_bitmask;
	u32 cell;
	u32 *cell_ptr;
	Vector2i cell_coord;
	Rng rng;
	Chunk *chunks[9];

	ChunkApi(Vector2i chunk_coord) :
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

	CellMaterial *cell_material() {
		return CellMaterial::materials[cell_material_idx()].ptr();
	}

	void bound_test(Vector2i coord) {
		TEST_ASSERT(coord.x >= 0, "coord.x is negative");
		TEST_ASSERT(coord.y >= 0, "coord.y is negative");
		TEST_ASSERT(coord.x < 96, "coord.x is too large");
		TEST_ASSERT(coord.y < 96, "coord.y is too large");
	}

	// Returns chunk idex and transform coord to be local to this chunk.
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
		other_cell_ptr = chunk_ptr->cells + (coord.x + coord.y * 32);
	}

	u32 *get_cell_ptr(Vector2i coord) {
		i32 chunk_idx = to_local(coord);
		return chunks[chunk_idx]->cells + (coord.x + coord.y * 32);
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
		CellMaterial *other_cell_material = CellMaterial::materials[Cell::material_idx(other)].ptr();
		return cell_material()->density > other_cell_material->density;
	}

	// Returns true if swapped.
	bool try_swap(Vector2i other_coord) {
		Chunk *other_chunk_ptr;
		u32 *other_cell_ptr;
		get_ptrs(other_coord, other_chunk_ptr, other_cell_ptr);
		u32 other = *other_cell_ptr;
		CellMaterial *other_cell_material = CellMaterial::materials[Cell::material_idx(other)].ptr();

		if (cell_material()->density > other_cell_material->density) {
			// todo: duplication on h movement

			Cell::set_updated(other, current_cell_updated_bitmask);
			current_cell_updated_bitmask = other_chunk_ptr->get_updated_mask(Grid::tick);
			Cell::set_updated(cell, current_cell_updated_bitmask);

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
		CellReactionPacked *reaction = nullptr;
		CellReactionPacked *reaction_end = nullptr;
		CellReaction::reactions_between(reaction, reaction_end, cell_material_idx(), other_material_idx, swap);

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
							current_cell_updated_bitmask,
							true);
					activate_neightbors(cell_coord);
				}

				if (other_material_idx != other_material_idx_out) {
					// todo: color
					*other_cell_ptr = Cell::build_cell(
							other_material_idx_out,
							other_chunk_ptr->get_updated_mask(Grid::tick),
							true);
					activate_neightbors(other_coord);
				}

				break;
			}

			reaction += 1;
		}
	}

	void step_cell(const Vector2i center_coord) {
		cell_ptr = center()->get_cell_ptr(center_coord);
		cell = *cell_ptr;
		current_cell_updated_bitmask = center()->current_cell_updated_bitmask;
		cell_coord = center_coord + Vector2i(32, 32);

		if (!Cell::is_active(cell)) {
			return;
		}

		if (Cell::is_updated(cell, current_cell_updated_bitmask)) {
			center()->activate_point(center_coord, false);
			return;
		}

		Cell::set_updated(cell, current_cell_updated_bitmask);
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
			if (!cell_material()->can_fall && orthogonal_velocity_i == 0) {
				Cell::clear_velocity(cell);
				*cell_ptr = cell;
				return;
			}
			f32 orthogonal_velocity = f32(orthogonal_velocity_i) / 255.0f;

			i32 v_dir;
			f32 vertical_acceleration = cell_material()->vertical_acceleration;
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

				f32 vel = orthogonal_velocity * cell_material()->vertical_velocity_max;
				i32 v_num = i32(vel) + i32(rng.gen_probability_f32(vel - Math::floor(vel)));
				for (i32 i = 0; i < v_num; i++) {
					if (!try_swap(Vector2i(cell_coord.x, cell_coord.y + v_dir))) {
						// Blocked. Transfer some vertical velocity to horizontal.
						orthogonal_velocity *= cell_material()->vertical_velocity_max / cell_material()->horizontal_velocity_max;
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
	bool were_active[9];

	for (i32 y = -1; y <= 1; y++) {
		for (i32 x = -1; x <= 1; x++) {
			u64 chunk_id = Grid::chunk_id(chunk_coord + Vector2i(x, y));
			Chunk **chunk_map_ptr = &Grid::chunks[chunk_id];
			if (*chunk_map_ptr == nullptr) {
				*chunk_map_ptr = new Chunk();

				// TODO: Generation
				// todo use static rng here
			}

			i32 chunk_api_chunk_idx = (x + 1) + (y + 1) * 3;
			chunk_api.chunks[chunk_api_chunk_idx] = *chunk_map_ptr;
			were_active[chunk_api_chunk_idx] = *chunk_map_ptr;
		}
	}

	chunk_api.center()->step_cell_updated_bitmask();

	if (chunk_api.center()->is_inactive()) {
		// Nothing to do.
		return;
	}

	u32 active_rows = chunk_api.center()->active_rows;
	u32 active_columns = chunk_api.center()->active_columns;
	chunk_api.center()->clear_active_rect();

	chunk_api.rng.mix(Grid::tick);
	i32 y_start = std::countr_zero(active_rows);
	i32 y_end = 32 - std::countl_zero(active_rows);

	i32 x_start_base = std::countr_zero(active_columns);
	i32 x_end_base = 32 - std::countl_zero(active_columns);
	i32 x_start;
	i32 x_end;
	i32 x_step;
	// Alternate iteration between left and right.
	if ((Grid::tick & 1) == 0) {
		x_step = -1;
		x_start = x_end_base - 1;
		x_end = x_start_base - 1;
	} else {
		x_step = 1;
		x_start = x_start_base;
		x_end = x_end_base;
	}

	// Iterate over each cell in the chunk.
	for (i32 y = y_start; y < y_end; y++) {
		if ((active_rows & (1u << y)) == 0) {
			continue;
		}

		i32 x = x_start;
		while (x != x_end) {
			chunk_api.step_cell(Vector2i(x, y));
			x += x_step;
		}
	}

	// TODO: handle active chunk changes
	// for (i32 y = -1; y <= 1; y++) {
	// 	for (i32 x = -1; x <= 1; x++) {
	// 		auto chunk = &Grid::chunks[Grid::chunk_id(chunk_coord + Vector2i(x, y))];
	// 		if (chunk->get() == nullptr) {
	// 			chunk->reset(new Chunk());

	// 			// TODO: Generation
	// 		}

	// 		i32 idx = (x + 1) + (y + 1) * 3;
	// 		rt.chunks[idx] = chunk->get();
	// 		were_active[idx] = chunk->get()->is_active();
	// 	}
	// }

	if (chunk_api.center()->is_inactive()) {
	}
}
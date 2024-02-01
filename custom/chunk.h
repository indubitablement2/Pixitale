#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "cell.hpp"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "preludes.h"
#include <bit>

// Coord is relative to top left cell.
class alignas(64) Chunk {
public:
	bool generated = false;

	i64 last_step_tick = -1;
	// Cell updated last time this chunk was stepped.
	u32 current_cell_updated_bitmask = 1 << Cell::Shifts::SHIFT_UPDATED;
	u32 next_cell_updated_bitmask = 2 << Cell::Shifts::SHIFT_UPDATED;

	u32 active_rows = MAX_U32;
	u32 active_columns = MAX_U32;

	i32 num_background_cells = 0;
	i32 num_particle_cells = 0;
	// todo: background cells
	u32 *background_cells = nullptr;
	// todo: particles
	u32 *particle_cells = nullptr;

	u32 cells[32 * 32];

	inline bool is_inactive() {
		return active_rows == 0;
	}

	inline bool is_active() {
		return !is_inactive();
	}

	inline bool is_row_active(u32 row) {
		return (active_rows & (1u << row)) != 0;
	}

	inline Rect2i active_rect() {
		i32 y_start = std::countr_zero(active_rows);
		i32 y_end = 32 - std::countl_zero(active_rows);
		i32 x_start = std::countr_zero(active_columns);
		i32 x_end = 32 - std::countl_zero(active_columns);

		return Rect2i(
				Vector2i(x_start, y_start),
				Vector2i(x_end - x_start, y_end - y_start));
	}

	inline void step_cell_updated_bitmask() {
		u32 temp = current_cell_updated_bitmask;
		current_cell_updated_bitmask = next_cell_updated_bitmask;
		next_cell_updated_bitmask = temp;
	}

	inline void bound_test(Vector2i coord) {
		TEST_ASSERT(coord.x >= 0, "coord.x is negative");
		TEST_ASSERT(coord.y >= 0, "coord.y is negative");
		TEST_ASSERT(coord.x < 32, "coord.x is too large");
		TEST_ASSERT(coord.y < 32, "coord.y is too large");
	}

	inline u32 *get_cell_ptr(Vector2i coord) {
		bound_test(coord);
		return cells + (coord.x + coord.y * 32);
	}

	inline u32 get_cell(Vector2i coord) {
		return *get_cell_ptr(coord);
	}

	// Does not modify active rect.
	inline void set_cell(Vector2i coord, u32 cell) {
		*get_cell_ptr(coord) = cell;
	}

	// Activate an area in bulk.
	// Rect needs to be within bound of this chunk.
	// Does not set cells to activated.
	inline void activate_rect(Rect2i rect) {
		bound_test(rect.position);
		bound_test(rect.get_end());

		active_rows |= ((1u << rect.size.y) - 1u) << rect.position.y;
		active_columns |= ((1u << rect.size.x) - 1u) << rect.position.x;
	}

	inline void activate_point(Vector2i coord, bool activate_cell) {
		bound_test(coord);

		active_rows |= 1u << coord.y;
		active_columns |= 1u << coord.x;

		if (activate_cell) {
			Cell::set_active(cells[coord.x + coord.y * 32]);
		}
	}

	inline void clear_active_rect() {
		active_rows = 0;
		active_columns = 0;
	}

	inline u32 get_updated_mask(u64 tick) {
		if (last_step_tick == tick) {
			// We have already stepped this chunk this tick.
			return current_cell_updated_bitmask;
		} else {
			// We have not stepped this chunk this tick and likely will.
			// Will prevent double stepping a cell.
			return next_cell_updated_bitmask;
		}
	}

	// Needs chunk and its 8 neighbors to exist in Grid::chunks,
	// They will be generated if needed.
	static void step_chunk(Vector2i chunk_coord);

	~Chunk() {
		if (background_cells != nullptr) {
			delete[] background_cells;
		}
		if (particle_cells != nullptr) {
			delete[] particle_cells;
		}
	}
};

static_assert(sizeof(Chunk) == 4160);
static_assert(sizeof(Chunk) % 64 == 0, "Alignment to cache line");

#endif
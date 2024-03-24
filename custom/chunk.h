#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "cell.hpp"
#include "core/math/rect2i.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "preludes.h"

// Coord is relative to first cell (top left).
class alignas(64) Chunk {
public:
	Vector2i chunk_coord;

	// -1 used to indicate that the chunk hasn't been generated.
	i64 last_step_tick = -1;

	u32 active_rows = MAX_U32;
	u32 active_columns = MAX_U32;

	u32 num_background_cell = 0;
	u32 *background = nullptr;

	u32 *cells_save = nullptr;

	u32 cells[32 * 32];

	inline i64 get_memory_usage() {
		i64 mem = sizeof(Chunk);
		if (background != nullptr) {
			mem += 32 * 32 * sizeof(u32);
		}
		if (cells_save != nullptr) {
			mem += 32 * 32 * sizeof(u32);
		}
		return mem;
	}

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
		i32 y_start = countr_zero(active_rows);
		i32 y_end = 32 - countl_zero(active_rows);
		i32 x_start = countr_zero(active_columns);
		i32 x_end = 32 - countl_zero(active_columns);

		return Rect2i(
				Vector2i(x_start, y_start),
				Vector2i(x_end - x_start, y_end - y_start));
	}

	inline void bound_test(Vector2i coord) {
		TEST_ASSERT(coord.x >= 0, "coord.x is negative");
		TEST_ASSERT(coord.y >= 0, "coord.y is negative");
		TEST_ASSERT(coord.x < 32, "coord.x is too large");
		TEST_ASSERT(coord.y < 32, "coord.y is too large");
	}

	inline u32 *get_cell_ptr(Vector2i coord) {
		bound_test(coord);
		return cells + coord.x + coord.y * 32;
	}

	inline u32 get_cell(Vector2i coord) {
		return *get_cell_ptr(coord);
	}

	// Does not modify active rect.
	inline void set_cell(Vector2i coord, u32 cell) {
		*get_cell_ptr(coord) = cell;
	}

	inline u32 get_background(Vector2i coord) {
		bound_test(coord);
		if (background == nullptr) {
			return 0;
		} else {
			return background[coord.x + coord.y * 32];
		}
	}

	inline void set_background(Vector2i coord, u32 cell) {
		bound_test(coord);
		if (background == nullptr) {
			if (cell == 0) {
				return;
			} else {
				background = new u32[32 * 32]{};
			}
		}

		u32 &ptr = background[coord.x + coord.y * 32];
		if (ptr != 0) {
			num_background_cell -= 1;
		}

		ptr = cell;
		if (cell != 0) {
			num_background_cell += 1;
		}
	}

	inline void activate_all(bool activate_cells) {
		active_rows = MAX_U32;
		active_columns = MAX_U32;

		if (activate_cells) {
			for (u32 i = 0; i < 32 * 32; i++) {
				Cell::set_active(cells[i]);
			}
		}
	}

	// Activate an area in bulk.
	// Rect needs to be within bound of this chunk.
	// Does not set cells to activated.
	inline void activate_rect(Rect2i rect) {
		bound_test(rect.position);
		bound_test(rect.get_end() - Vector2i(1, 1));

		active_rows |= u32((1uLL << rect.size.y) - 1uLL) << rect.position.y;
		active_columns |= u32((1uLL << rect.size.x) - 1uLL) << rect.position.x;
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

	// Needs chunk and its 8 neighbors to exist in Grid::chunks,
	// They will be generated if needed.
	static void step_chunk(Vector2i chunk_coord);

	~Chunk() {
		if (background != nullptr) {
			delete[] background;
		}
		if (cells_save != nullptr) {
			delete[] cells_save;
		}
	}
};

static_assert(sizeof(Chunk) == 4160);
static_assert(sizeof(Chunk) % 64 == 0, "Alignment to cache line");

#endif
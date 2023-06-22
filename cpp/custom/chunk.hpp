#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "preludes.h"

#include "grid.h"
#include <bit>

using namespace godot;

namespace Chunk {

struct ChunkActiveRect {
	i32 x_start;
	i32 x_end;
	i32 y_start;
	i32 y_end;
};

inline u32 get_rows(u64 chunk) {
	return (u32)chunk;
}

inline u32 get_columns(u64 chunk) {
	return (u32)(chunk >> 32);
}

inline ChunkActiveRect active_rect(u64 chunk) {
	ChunkActiveRect rect;

	if (chunk == 0) {
		rect.x_start = 0;
		rect.x_end = 0;
		rect.y_start = 0;
		rect.y_end = 0;
		return rect;
	}

	u32 rows = get_rows(chunk);
	u32 columns = get_columns(chunk);

	test_assert(rows > 0, "Rows is 0");
	test_assert(columns > 0, "Columns is 0");

	rect.x_start = std::countr_zero(columns);
	rect.x_end = 32 - std::countl_zero(columns);
	rect.y_start = std::countr_zero(rows);
	rect.y_end = 32 - std::countl_zero(rows);

	return rect;
}

// Rect needs to be within a single chunk.
inline void unsafe_activate_rect(
		u64 &chunk,
		i32 x_offset,
		i32 y_offset,
		u64 width,
		u64 height) {
	test_assert(x_offset >= 0, "x_offset is negative");
	test_assert(y_offset >= 0, "y_offset is negative");
	test_assert(x_offset + width <= 32, "x_offset + width is too large");
	test_assert(y_offset + height <= 32, "y_offset + height is too large");
	test_assert(width > 0, "width is 0");
	test_assert(height > 0, "height is 0");

	chunk |= ((1uLL << height) - 1uLL) << y_offset; // Set rows
	chunk |= ((1uLL << width) - 1uLL) << (x_offset + 32); // Set columns

	test_assert(get_rows(chunk) != 0, "a row should've been activated");
	test_assert(get_columns(chunk) != 0, "a column should've been activated");
}

inline void activate_point(i32 x, i32 y) {
	i32 local_x = x & 31;
	i32 local_y = y & 31;
	u64 *chunk_ptr = Grid::chunks + (x >> 5) + (y >> 5) * Grid::chunks_width;
	unsafe_activate_rect(*chunk_ptr, local_x, local_y, 1, 1);
}

} // namespace Chunk

#endif
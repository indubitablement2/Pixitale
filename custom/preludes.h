#ifndef PRELUDES_HPP
#define PRELUDES_HPP

// todo:
// // add reactions property
// // add cell material image property
// rename all godot class with Pixitale prefix
// // remove prints from tests

#include "core/error/error_macros.h"
#include "core/math/rect2i.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

const u8 MAX_U8 = UINT8_MAX;
const u16 MAX_U16 = UINT16_MAX;
const u32 MAX_U32 = UINT32_MAX;
const u64 MAX_U64 = UINT64_MAX;

const i8 MIN_I8 = INT8_MIN;
const i8 MAX_I8 = INT8_MAX;
const i16 MIN_I16 = INT16_MIN;
const i16 MAX_I16 = INT16_MAX;
const i32 MIN_I32 = INT32_MIN;
const i32 MAX_I32 = INT32_MAX;
const i64 MIN_I64 = INT64_MIN;
const i64 MAX_I64 = INT64_MAX;

/// Crash if condition is false.
#ifdef TOOLS_ENABLED
#define TEST_ASSERT(m_cond, m_msg) CRASH_COND_MSG(!(m_cond), m_msg)
#else
#define TEST_ASSERT(m_cond, m_msg) ((void)0)
#endif

// Round toward negative infinity instead of 0.
inline i32 div_floor(i32 numerator, i32 denominator) {
	if (numerator >= 0) {
		return numerator / denominator;
	} else {
		return -1 - (-1 - numerator) / denominator;
	}
}

// Modulo which handle negative numbers instead of returning the remainder.
inline i32 mod_neg(i32 numerator, i32 denominator) {
	i32 mod = numerator % denominator;
	return mod >= 0 ? mod : mod + denominator;
}

// Handle end < start.
struct Iter1D {
	i32 _start;
	i32 _end;
	i32 _step;

	i32 current;

	inline Iter1D(i32 p_start, i32 p_end, i32 p_step, i32 p_current) :
			_start(p_start),
			_end(p_end),
			_step(p_step),
			current(p_current) {}

	inline Iter1D(i32 start, i32 end) :
			_start(start),
			_end(end),
			_step(end >= start ? 1 : -1),
			current(start - _step) {}

	// Iterate from start to end (excluded).
	// `[start..end[`.
	inline static Iter1D exclusive(i32 start, i32 end) {
		return Iter1D(start, end);
	}

	// Iterate from start to end (inclusive).
	// `[start..end]`.
	inline static Iter1D inclusive(i32 start, i32 end) {
		i32 step = end >= start ? 1 : -1;
		return Iter1D(start, end + step, step, start - step);
	}

	inline void restart() {
		current = _start - _step;
	}

	inline bool next() {
		if (current == _end) {
			return false;
		}

		current += _step;

		if (current == _end) {
			return false;
		} else {
			return true;
		}
	}
};

struct Iter2D {
	Vector2i _start;
	Vector2i _end;
	Vector2i _step;

	Vector2i coord;

	inline Iter2D(){};

	inline Iter2D(Vector2i start, Vector2i end, Vector2i step, Vector2i p_current) :
			_start(start),
			_end(end),
			_step(step),
			coord(p_current) {}

	// Iterate from start to end (excluded).
	// `[start..end[`.
	inline Iter2D(Vector2i start, Vector2i end) :
			_start(start),
			_end(end),
			_step(Vector2i(end.x >= start.x ? 1 : -1, end.y >= start.y ? 1 : -1)),
			coord(Vector2i(start.x - _step.x, start.y)) {
		if (_start.x == _end.x) {
			_end.y = _start.y;
		}
	}

	// // Iterate from start to end (excluded).
	// // `[start..end[`.
	// inline static Iter2D exclusive(Vector2i start, Vector2i end) {
	// 	return Iter2D(start, end);
	// }

	inline void reset() {
		coord = Vector2i(_start.x - _step.x, _start.y);
	}

	inline bool next() {
		if (coord.y == _end.y) {
			return false;
		}

		coord.x += _step.x;

		if (coord.x == _end.x) {
			coord.x = _start.x;
			coord.y += _step.y;

			if (coord.y == _end.y) {
				return false;
			}
		}

		return true;
	}
};

struct VectorLine {
public:
	Vector2i current;

private:
	Vector2i end;
	Vector2 next_current;
	Vector2 slope;
	bool finished;
	bool really_finished;

public:
	inline VectorLine(Vector2i to) {
		end = to;
		next_current = Vector2(0.5f, 0.5f);
		slope = Vector2(f32(end.x) + 0.5f, f32(end.y) + 0.5f).normalized();
		finished = false;
		really_finished = false;
	}

	inline bool next() {
		if (finished) {
			if (!really_finished) {
				current = Vector2i(next_current.x, next_current.y);
				really_finished = true;
				return true;
			} else {
				return false;
			}
		} else {
			current = Vector2i(i32(next_current.x), i32(next_current.y));
			next_current += slope;
			if (Vector2i(next_current.x, next_current.y) == end) {
				finished = true;
			}
			return true;
		}
	}
};

struct Bresenham {
public:
	Vector2i current;

private:
	i32 x;
	i32 y;
	i32 dx;
	i32 dy;
	i32 x1;
	i32 diff;
	i32 octant;

	inline static i32 octant_from_points(Vector2i start, Vector2i end) {
		i32 dx = end.x - start.x;
		i32 dy = end.y - start.y;

		i32 octant = 0;

		if (dy < 0) {
			dx = -dx;
			dy = -dy;
			octant += 4;
		}

		if (dx < 0) {
			i32 tmp = dx;
			dx = dy;
			dy = -tmp;
			octant += 2;
		}

		if (dx < dy) {
			octant += 1;
		}

		return octant;
	}

	inline static Vector2i to_octant_0(i32 octant, Vector2i p) {
		switch (octant) {
			case 0:
				return Vector2i(p.x, p.y);
			case 1:
				return Vector2i(p.y, p.x);
			case 2:
				return Vector2i(p.y, -p.x);
			case 3:
				return Vector2i(-p.x, p.y);
			case 4:
				return Vector2i(-p.x, -p.y);
			case 5:
				return Vector2i(-p.y, -p.x);
			case 6:
				return Vector2i(-p.y, p.x);
			default:
				return Vector2i(p.x, -p.y);
		}
	}

	inline static Vector2i from_octant_0(i32 octant, Vector2i p) {
		switch (octant) {
			case 0:
				return Vector2i(p.x, p.y);
			case 1:
				return Vector2i(p.y, p.x);
			case 2:
				return Vector2i(p.y, -p.x);
			case 3:
				return Vector2i(-p.x, p.y);
			case 4:
				return Vector2i(-p.x, -p.y);
			case 5:
				return Vector2i(-p.y, -p.x);
			case 6:
				return Vector2i(-p.y, p.x);
			default:
				return Vector2i(p.x, -p.y);
		}
	}

public:
	inline Bresenham(Vector2i start, Vector2i end) {
		octant = octant_from_points(start, end);

		start = to_octant_0(octant, start);
		end = to_octant_0(octant, end);

		dx = end.x - start.x;
		dy = end.y - start.y;

		x = start.x;
		y = start.y;
		x1 = end.x;
		diff = dy - dx;
	}

	// Get the next point without checking if we are past `end`.
	inline void advance() {
		Vector2i p = Vector2i(x, y);

		if (diff >= 0) {
			y += 1;
			diff -= dx;
		}
		diff += dy;

		// loop inc
		x += 1;

		current = from_octant_0(octant, p);
	}

	// [start..end[
	inline bool next_exclusive(Vector2i &out) {
		if (x >= x1) {
			return false;
		} else {
			advance();
			return true;
		}
	}

	// [start..end]
	inline bool next_inclusive(Vector2i &out) {
		if (x > x1) {
			return false;
		} else {
			advance();
			return true;
		}
	}
};

// Transform global coord to chunk coord + local coord (`[0..32]`).
struct ChunkLocalCoord {
	Vector2i chunk_coord;
	Vector2i local_coord;

	inline ChunkLocalCoord(){};

	inline ChunkLocalCoord(Vector2i coord) :
			chunk_coord(Vector2i(div_floor(coord.x, 32), div_floor(coord.y, 32))),
			local_coord(Vector2i(mod_neg(coord.x, 32), mod_neg(coord.y, 32))) {}

	inline ChunkLocalCoord(Vector2i p_chunk_coord, Vector2i p_local_coord) :
			chunk_coord(p_chunk_coord),
			local_coord(p_local_coord) {}
};

// Iterate over all the chunks which intersect the rect.
struct IterChunk {
	ChunkLocalCoord _start;
	ChunkLocalCoord _end;

	Vector2i chunk_coord;
	Vector2i local_coord_start;
	Vector2i local_coord_end;

	inline IterChunk(Rect2i rect) :
			_start(ChunkLocalCoord(rect.position)),
			_end(ChunkLocalCoord(rect.get_end())),
			chunk_coord(Vector2i(_start.chunk_coord.x - 1, _start.chunk_coord.y)) {}

	// Return true if there is a next chunk
	// and update chunk_coord and local_coord_start/end.
	inline bool next() {
		if (chunk_coord.y >= _end.chunk_coord.y) {
			return false;
		}

		chunk_coord.x += 1;

		if (chunk_coord.x >= _end.chunk_coord.x) {
			chunk_coord.x = _start.chunk_coord.x;
			chunk_coord.y += 1;
		}

		if (chunk_coord.x == _start.chunk_coord.x) {
			local_coord_start.x = _start.local_coord.x;
		} else {
			local_coord_start.x = 0;
		}
		if (chunk_coord.x + 1 >= _end.chunk_coord.x) {
			local_coord_end.x = _end.local_coord.x;
		} else {
			local_coord_end.x = 32;
		}
		if (chunk_coord.y == _start.chunk_coord.y) {
			local_coord_start.y = _start.local_coord.y;
		} else {
			local_coord_start.y = 0;
		}
		if (chunk_coord.y + 1 >= _end.chunk_coord.y) {
			local_coord_end.y = _end.local_coord.y;
		} else {
			local_coord_end.y = 32;
		}

		return true;
	}

	inline Iter2D local_iter() {
		return Iter2D(local_coord_start, local_coord_end);
	}
};

#endif // PRELUDES_HPP
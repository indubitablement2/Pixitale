#ifndef PRELUDES_HPP
#define PRELUDES_HPP

#include "core/error/error_macros.h"
#include "core/math/rect2i.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include <limits>

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

const f32 INF_F32 = std::numeric_limits<f32>::infinity();
const f32 NEG_INF_F32 = -std::numeric_limits<f32>::infinity();

/// Crash if condition is false.
#ifdef TOOLS_ENABLED
#define TEST_ASSERT(m_cond, m_msg) CRASH_COND_MSG(!(m_cond), m_msg)
#else
#define TEST_ASSERT(m_cond, m_msg) ((void)0)
#endif

// Round toward negative infinity instead of 0.
inline i32 div_floor(i32 numerator, i32 denominator) {
	TEST_ASSERT(denominator > 0, "denominator is not greater than 0");

	if (numerator >= 0) {
		return numerator / denominator;
	} else {
		return -1 - (-1 - numerator) / denominator;
	}
}

inline Vector2i div_floor(Vector2i numerator, Vector2i denominator) {
	return Vector2i(
			div_floor(numerator.x, denominator.x),
			div_floor(numerator.y, denominator.y));
}

inline Vector2i div_floor(Vector2i numerator, i32 denominator) {
	return Vector2i(
			div_floor(numerator.x, denominator),
			div_floor(numerator.y, denominator));
}

// Modulo which handle negative numbers instead of returning the remainder.
inline i32 mod_neg(i32 numerator, i32 denominator) {
	TEST_ASSERT(denominator > 0, "denominator is not greater than 0");

	i32 mod = numerator % denominator;
	// return mod >= 0 ? mod : mod + denominator;
	if (mod >= 0) {
		return mod;
	} else {
		return mod + denominator;
	}
}

inline Vector2i mod_neg(Vector2i numerator, Vector2i denominator) {
	return Vector2i(
			mod_neg(numerator.x, denominator.x),
			mod_neg(numerator.y, denominator.y));
}

inline Vector2i mod_neg(Vector2i numerator, i32 denominator) {
	return Vector2i(
			mod_neg(numerator.x, denominator),
			mod_neg(numerator.y, denominator));
}

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
			coord(p_current) {
		TEST_ASSERT(step.x == -1 || step.x == 1, "step.x is not -1 or 1");
		TEST_ASSERT(step.y == -1 || step.y == 1, "step.y is not -1 or 1");
		TEST_ASSERT(start != end, "start is equal to end");
	}

	// Only support positive step.
	inline Iter2D(Vector2i size) :
			_start(Vector2i(0, 0)),
			_end(size),
			_step(Vector2i(1, 1)),
			coord(Vector2i(-1, 0)) {
		if (_start.x == _end.x) {
			_end.y = _start.y + 1;
		}

		TEST_ASSERT(_start.x <= _end.x, "start.x is not less than end.x");
		TEST_ASSERT(_start.y <= _end.y, "start.y is not less or eq than end.y");
	}

	// Only support positive step.
	inline Iter2D(Rect2i rect) :
			_start(rect.position),
			_end(rect.get_end()),
			_step(Vector2i(1, 1)),
			coord(Vector2i(_start.x - _step.x, _start.y)) {
		if (_start.x == _end.x) {
			_end.y = _start.y + 1;
		}

		TEST_ASSERT(_start.x <= _end.x, "start.x is not less than end.x");
		TEST_ASSERT(_start.y <= _end.y, "start.y is not less or eq than end.y");
	}

	// Iterate from start to end (excluded).
	// `[start..end[`.
	inline Iter2D(Vector2i start, Vector2i end) :
			_start(start),
			_end(end),
			_step(Vector2i(end.x >= start.x ? 1 : -1, end.y >= start.y ? 1 : -1)),
			coord(Vector2i(start.x - _step.x, start.y)) {
		if (_start.x == _end.x || _start.y == _end.y) {
			_end.y = _start.y + _step.y;
			_end.x = _start.x;
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

	// Loop back to the start after returning false.
	inline bool next() {
		coord.x += _step.x;

		if (coord.x == _end.x) {
			coord.x = _start.x;
			coord.y += _step.y;

			if (coord.y == _end.y) {
				coord.y = _start.y;
				coord.x -= _step.x;
				return false;
			}
		}

		return true;
	}
};

struct IterLine {
	f32 p;
	f32 num_points;
	Vector2 to;

	inline IterLine(){};

	inline IterLine(Vector2i p_to, const bool include_start = true) :
			p(include_start ? -1.0f : 0.0f),
			num_points(f32(MAX(ABS(p_to.x), ABS(p_to.y)))),
			to(Vector2(p_to) + Vector2(0.5f, 0.5f)) {}

	inline bool next() {
		p += 1.0f;
		return p <= num_points;
	}

	inline Vector2 currentf() {
		if (num_points == 0.0f) {
			return Vector2(0.0f, 0.0f);
		} else {
			return (p / num_points) * to;
		}
	}

	inline Vector2i currenti() {
		return Vector2i(currentf().floor());
	}

	inline void reset(const bool include_start = true) {
		p = include_start ? -1.0f : 0.0f;
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

// Transform global coord to chunk coord + local coord (`[0..32[`).
struct ChunkLocalCoord {
	Vector2i chunk_coord;
	Vector2i local_coord;

	inline ChunkLocalCoord(){};

	inline ChunkLocalCoord(Vector2i coord) :
			chunk_coord(div_floor(coord, 32)),
			local_coord(mod_neg(coord, 32)) {}

	inline ChunkLocalCoord(Vector2i p_chunk_coord, Vector2i p_local_coord) :
			chunk_coord(p_chunk_coord),
			local_coord(p_local_coord) {}

	inline Vector2i coord() {
		return chunk_coord * 32 + local_coord;
	}

	inline bool operator==(const ChunkLocalCoord &other) {
		return chunk_coord == other.chunk_coord && local_coord == other.local_coord;
	}

	inline bool operator!=(const ChunkLocalCoord &other) {
		return chunk_coord != other.chunk_coord || local_coord != other.local_coord;
	}
};

// Iterate over all the chunks which intersect the rect.
struct IterChunk {
	ChunkLocalCoord _start;
	ChunkLocalCoord _end;

	Vector2i chunk_coord;
	Vector2i local_coord_start;
	Vector2i local_coord_end;

	inline IterChunk(){};

	inline IterChunk(Rect2i rect) :
			_start(ChunkLocalCoord(rect.position)),
			_end(ChunkLocalCoord(rect.get_end())),
			chunk_coord(Vector2i(_start.chunk_coord.x - 1, _start.chunk_coord.y)) {
		// if (rect.size.x <= 0 || rect.size.y <= 0) {
		// 	chunk_coord = _end.chunk_coord + Vector2i(1, 1);
		// }

		if (_end.local_coord.x == 0) {
			_end.chunk_coord.x -= 1;
			_end.local_coord.x = 32;
		}
		if (_end.local_coord.y == 0) {
			_end.chunk_coord.y -= 1;
			_end.local_coord.y = 32;
		}

		if (_start.chunk_coord == _end.chunk_coord && _start.local_coord >= _end.local_coord) {
			_end.chunk_coord = _start.chunk_coord - Vector2i(1, 1);
		}
	}

	// Return true if there is a next chunk
	// and update chunk_coord and local_coord_start/end.
	inline bool next() {
		chunk_coord.x += 1;

		if (chunk_coord.x > _end.chunk_coord.x) {
			chunk_coord.x = _start.chunk_coord.x;
			chunk_coord.y += 1;

			if (chunk_coord.y > _end.chunk_coord.y) {
				chunk_coord.x -= 1;
				chunk_coord.y -= _start.chunk_coord.y;
				return false;
			}
		}

		if (chunk_coord.x == _start.chunk_coord.x) {
			local_coord_start.x = _start.local_coord.x;
		} else {
			local_coord_start.x = 0;
		}
		if (chunk_coord.x >= _end.chunk_coord.x) {
			local_coord_end.x = _end.local_coord.x;
		} else {
			local_coord_end.x = 32;
		}
		if (chunk_coord.y == _start.chunk_coord.y) {
			local_coord_start.y = _start.local_coord.y;
		} else {
			local_coord_start.y = 0;
		}
		if (chunk_coord.y >= _end.chunk_coord.y) {
			local_coord_end.y = _end.local_coord.y;
		} else {
			local_coord_end.y = 32;
		}

		return true;
	}

	inline Iter2D local_iter() {
		return Iter2D(local_coord_start, local_coord_end);
	}

	inline Rect2i local_rect() {
		return Rect2i(local_coord_start, local_coord_end - local_coord_start);
	}

	inline void reset() {
		chunk_coord = Vector2i(_start.chunk_coord.x - 1, _start.chunk_coord.y);
	}
};

#endif // PRELUDES_HPP
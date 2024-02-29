#ifndef CELL_HPP
#define CELL_HPP

#include "core/math/color.h"
#include "core/math/vector2.h"
#include "core/typedefs.h"
#include "preludes.h"

struct CellColor {
	f32 r;
	f32 g;
	f32 b;

	inline CellColor() :
			r(0.0f), g(0.0f), b(0.0f) {}

	inline CellColor(f32 rr, f32 gg, f32 bb) :
			r(rr), g(gg), b(bb) {}

	inline CellColor(const Color &color) :
			r(color.r), g(color.g), b(color.b) {}

	inline Color to_color() const {
		return Color(r, g, b);
	}

	inline void average(CellColor other) {
		r = (r + other.r) / 2.0f;
		g = (g + other.g) / 2.0f;
		b = (b + other.b) / 2.0f;
	}

	inline void blend(CellColor other) {
		r *= other.r;
		g *= other.g;
		b *= other.b;
	}

	inline void add(CellColor other) {
		r += other.r;
		g += other.g;
		b += other.b;
	}
};

namespace Cell {

const f32 VELOCITY_MAX = 8.0f;

enum Shifts {
	SHIFT_UPDATED = 14,
	SHIFT_ACTIVE = 16,
	SHIFT_COLOR = 17,
};

// material_idx: 0..14
// updated: 14..16
// alternate between 1,2 and 3. 0 is always not updated (default for new cell).
// active: 16
// color: 17..32
// --
// x vel: 32..48
// y vel: 48..64
enum Masks {
	MASK_MATERIAL = 16383u,
	MASK_UPDATED = 3u << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1u << Shifts::SHIFT_ACTIVE,
	MASK_COLOR = 32767u << Shifts::SHIFT_COLOR,
};

inline u32 material_idx(const u32 cell) {
	return cell & Masks::MASK_MATERIAL;
}

inline void set_material_idx(u32 &cell, const u32 material_idx) {
	cell = (cell & ~Masks::MASK_MATERIAL) | material_idx | Masks::MASK_ACTIVE;
}

inline bool is_updated(const u32 cell, const u32 updated_mask) {
	return (cell & Masks::MASK_UPDATED) == updated_mask;
}

inline void set_updated(u32 &cell, const u32 updated_mask) {
	cell = (cell & ~Masks::MASK_UPDATED) | updated_mask;
}

inline bool is_active(const u32 cell) {
	return cell & Masks::MASK_ACTIVE;
}

// When inactive, set updated bit to 0 (never skip when it return to active).
inline void set_active(u32 &cell, const bool active = true) {
	if (active) {
		cell |= Masks::MASK_ACTIVE;
	} else {
		cell &= ~Masks::MASK_ACTIVE;
		cell &= ~Masks::MASK_UPDATED;
	}
}

inline CellColor color(const u32 cell) {
	u32 color_data = (cell & Masks::MASK_COLOR) >> Shifts::SHIFT_COLOR;
	return CellColor(
			f32(color_data & 31u) / 31.0f,
			f32((color_data >> 5) & 31u) / 31.0f,
			f32((color_data >> 10)) / 31.0f);
}

inline void set_color(u32 &cell, const CellColor color) {
	u32 color_data = u32(CLAMP(color.r, 0.0f, 1.0f) * 31.0f) |
			(u32(CLAMP(color.g, 0.0f, 1.0f) * 31.0f) << 5) |
			(u32(CLAMP(color.b, 0.0f, 1.0f) * 31.0f) << 10);
	cell = (cell & ~Masks::MASK_COLOR) | (color_data << Shifts::SHIFT_COLOR);
}

inline u32 build_cell(
		const u32 material_idx,
		const u32 updated_mask = 0,
		const bool active = true) {
	u32 cell = material_idx;
	set_updated(cell, updated_mask);
	set_active(cell, active);
	return cell;
}

inline Vector2 velocity(const u32 vel) {
	return Vector2(
			f32(vel & 65535u) * (VELOCITY_MAX * 2.0f / 65535.0f) - VELOCITY_MAX,
			f32(vel >> 16) * (VELOCITY_MAX * 2.0f / 65535.0f) - VELOCITY_MAX);
}

inline void set_velocity(u32 &vel, const Vector2 velocity) {
	u32 x = u32(CLAMP(velocity.x / (VELOCITY_MAX * 2.0f) + 0.5f, 0.0f, 1.0f) * 65535.0f);
	u32 y = u32(CLAMP(velocity.y / (VELOCITY_MAX * 2.0f) + 0.5f, 0.0f, 1.0f) * 65535.0f);
	vel = x | (y << 16);
}

} // namespace Cell

#endif
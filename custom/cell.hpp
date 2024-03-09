#ifndef CELL_HPP
#define CELL_HPP

#include "core/math/color.h"
#include "core/typedefs.h"
#include "preludes.h"

namespace Cell {

enum Shifts {
	SHIFT_MOVEMENT = 12,
	SHIFT_UPDATED = 14,
	SHIFT_ACTIVE = 16,
	SHIFT_COLOR = 17,
	SHIFT_RED = 17,
	SHIFT_GREEN = 22,
	SHIFT_BLUE = 27,
};

// material_idx: 0..12
// movement: 12..14
// 0 none, 1 left, 2 unused, 3 right
// updated: 14..16
// alternate between 1,2 and 3. 0 is always not updated (default for new cell).
// active: 16
// color: 17..32
enum Masks {
	MASK_MATERIAL = 4095u,
	MASK_MOVEMENT = 3u << Shifts::SHIFT_MOVEMENT,
	MASK_UPDATED = 3u << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1u << Shifts::SHIFT_ACTIVE,
	MASK_COLOR = 32767u << Shifts::SHIFT_COLOR,
	MASK_RED = 31u << Shifts::SHIFT_RED,
	MASK_GREEN = 31u << Shifts::SHIFT_GREEN,
	MASK_BLUE = 31u << Shifts::SHIFT_BLUE,
};

inline u32 material_idx(const u32 cell) {
	return cell & Masks::MASK_MATERIAL;
}

inline void set_material_idx(u32 &cell, const u32 material_idx) {
	cell = (cell & ~Masks::MASK_MATERIAL) | material_idx | Masks::MASK_ACTIVE;
}

// -2: none, -1: left, 0: vertical, 1: right
inline i32 movement(const u32 cell) {
	return i32((cell & Masks::MASK_MOVEMENT) >> Shifts::SHIFT_MOVEMENT) - 2;
}

inline void set_movement(u32 &cell, const i32 movement) {
	TEST_ASSERT(movement >= -2 && movement <= 1, "movement must be between -2 and 1");
	cell &= ~Masks::MASK_MOVEMENT;
	cell |= u32(movement + 2) << Shifts::SHIFT_MOVEMENT;
}

inline bool is_updated(const u32 cell, const u32 updated_mask) {
	return (cell & Masks::MASK_UPDATED) == updated_mask;
}

inline void set_updated(u32 &cell, const u32 updated_mask) {
	cell = (cell & ~Masks::MASK_UPDATED) | updated_mask;
}

inline void clear_updated(u32 &cell) {
	cell = (cell & ~Masks::MASK_UPDATED);
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
	}
}

// Store in a way that 0.5f is 0u;
// 0.0f -> 31
// 0.4f -> 18
// 0.5f -> 0
// 1.0f -> 15
inline u32 store_channel(f32 v) {
	v = CLAMP(v, 0.0f, 1.0f);

	v -= 0.5f;
	if (v < 0.0f) {
		v = -v + 0.5f;
		v = MAX(v, 0.52f);
	}
	v *= 31.0f;

	return u32(v);
}

inline f32 get_channel(const u32 v) {
	f32 f = f32(v);

	f /= 31.0f;
	if (f > 0.5f) {
		f = -f + 0.5f;
	}
	f += 0.5f;

	return f;
}

inline Color color(const u32 cell) {
	u32 color_data = (cell & Masks::MASK_COLOR) >> Shifts::SHIFT_COLOR;
	return Color(
			get_channel(color_data & 31u),
			get_channel((color_data >> 5) & 31u),
			get_channel(color_data >> 10));
}

inline void set_color(u32 &cell, const Color color) {
	u32 color_data = (store_channel(color.r)) |
			(store_channel(color.g) << 5) |
			(store_channel(color.b) << 10);

	cell &= ~Masks::MASK_COLOR;
	cell |= color_data << Shifts::SHIFT_COLOR;
}

// Meant for new cell.
inline void set_color_value(u32 &cell, const u32 value) {
	TEST_ASSERT(value < 32, "darken must be less than 32");

	u32 color_data = value | (value << 5) | (value << 10);

	cell |= color_data << Shifts::SHIFT_COLOR;
}

static_assert(
		Masks::MASK_MATERIAL +
				Masks::MASK_MOVEMENT +
				Masks::MASK_UPDATED +
				Masks::MASK_ACTIVE +
				Masks::MASK_COLOR ==
		MAX_U32);

static_assert(
		Masks::MASK_RED +
				Masks::MASK_GREEN +
				Masks::MASK_BLUE ==
		Masks::MASK_COLOR);

} // namespace Cell

#endif
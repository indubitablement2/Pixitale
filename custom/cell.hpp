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

// todo 0u should be 0.5f

inline Color color(const u32 cell) {
	u32 color_data = (cell & Masks::MASK_COLOR) >> Shifts::SHIFT_COLOR;
	return Color(
			f32((color_data & 31u) ^ 31u) / 31.0f,
			f32(((color_data >> 5) & 31u) ^ 31u) / 31.0f,
			f32((color_data >> 10) ^ 31u) / 31.0f);
}

inline void set_color(u32 &cell, const Color color) {
	u32 color_data = (u32(CLAMP(color.r, 0.0f, 1.0f) * 31.0f) ^ 31u) |
			((u32(CLAMP(color.g, 0.0f, 1.0f) * 31.0f) ^ 31u) << 5) |
			((u32(CLAMP(color.b, 0.0f, 1.0f) * 31.0f) ^ 31u) << 10);
	cell = (cell & ~Masks::MASK_COLOR) | (color_data << Shifts::SHIFT_COLOR);
}

inline void darken(u32 &cell, const u32 amount) {
	u32 r = (cell >> SHIFT_RED) & 31u;
	u32 g = (cell >> SHIFT_GREEN) & 31u;
	u32 b = (cell >> SHIFT_BLUE) & 31u;

	r = MIN(r + amount, 31u);
	g = MIN(g + amount, 31u);
	b = MIN(b + amount, 31u);

	cell &= ~MASK_COLOR;

	cell |= r << SHIFT_RED;
	cell |= g << SHIFT_GREEN;
	cell |= b << SHIFT_BLUE;
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
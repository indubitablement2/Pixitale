#ifndef CELL_HPP
#define CELL_HPP

#include "preludes.h"

namespace Cell {

enum Shifts {
	SHIFT_MOVEMENT = 12,
	SHIFT_UPDATED = 14,
	SHIFT_ACTIVE = 16,
	SHIFT_UNUSED = 17,
	SHIFT_FLOW = 18,
	SHIFT_DARKEN = 20,
	SHIFT_COLOR = 26,
};

// material_idx: 0..12
// movement: 12..14
// 0 none, 1 left, 2 unused, 3 right
// updated: 14..16
// alternate between 1,2 and 3. 0 is always not updated (default for new cell).
// active: 16
// unused: 17
// flow: 18..20
// Keep moving in the same direction until flow is 0. Makes fluid disperse faster.
// darken: 20..26
// color: 26..32
enum Masks {
	MASK_MATERIAL = 4095u,
	MASK_MOVEMENT = 3u << Shifts::SHIFT_MOVEMENT,
	MASK_UPDATED = 3u << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1u << Shifts::SHIFT_ACTIVE,
	MASK_UNUSED = 1u << Shifts::SHIFT_UNUSED,
	MASK_FLOW = 3u << Shifts::SHIFT_FLOW,
	MASK_DARKEN = 63u << Shifts::SHIFT_DARKEN,
	MASK_COLOR = 63u << Shifts::SHIFT_COLOR,
};

inline u32 material_idx(const u32 cell) {
	return cell & Masks::MASK_MATERIAL;
}

inline void set_material_idx(u32 &cell, const u32 material_idx) {
	TEST_ASSERT(material_idx < 4096, "material_idx must be less than 4096");
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

inline u32 flow(const u32 cell) {
	return (cell & Masks::MASK_FLOW) >> Shifts::SHIFT_FLOW;
}

inline void set_flow(u32 &cell, const u32 flow) {
	TEST_ASSERT(flow < 4, "flow must be less than 8");
	cell &= ~Masks::MASK_FLOW;
	cell |= flow << Shifts::SHIFT_FLOW;
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

inline void set_darken(u32 &cell, const u32 darken) {
	TEST_ASSERT(darken < 64, "darken must be less than 64");
	cell &= ~Masks::MASK_DARKEN;
	cell |= darken << Shifts::SHIFT_DARKEN;
}

inline u32 darken(const u32 cell) {
	return (cell & Masks::MASK_DARKEN) >> Shifts::SHIFT_DARKEN;
}

inline void set_color(u32 &cell, const u32 color) {
	TEST_ASSERT(color < 64, "color must be less than 64");
	cell &= ~Masks::MASK_COLOR;
	cell |= color << Shifts::SHIFT_COLOR;
}

inline u32 color(const u32 cell) {
	return (cell & Masks::MASK_COLOR) >> Shifts::SHIFT_COLOR;
}

static_assert(
		Masks::MASK_MATERIAL +
				Masks::MASK_MOVEMENT +
				Masks::MASK_UPDATED +
				Masks::MASK_ACTIVE +
				Masks::MASK_UNUSED +
				Masks::MASK_FLOW +
				Masks::MASK_DARKEN +
				Masks::MASK_COLOR ==
		MAX_U32);

} // namespace Cell

#endif
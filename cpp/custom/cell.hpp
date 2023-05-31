#ifndef CELL_HPP
#define CELL_HPP

#include "preludes.h"

namespace Cell {

enum Movement {
	MOVEMENT_SOLID,
	MOVEMENT_POWDER,
	MOVEMENT_LIQUID,
	MOVEMENT_GAS,
};

enum Collision {
	COLLISION_NONE,
	COLLISION_SOLID,
	COLLISION_PLATFORM,
	COLLISION_LIQUID,
};

enum Shifts {
	SHIFT_UPDATED = 12,
	SHIFT_ACTIVE = 14,
	SHIFT_HUE = 24,
	SHIFT_VALUE = 28,
};

// MASK_MATERIAL = 0..12
// MASK_UPDATED = 12..14
// MASK_ACTIVE = 14..15
// unused = 15..24
// MASK_HUE = 24..28
// MASK_VALUE = 28..32
enum Masks {
	MASK_MATERIAL = 0xFFF,
	// Alternate between 1, 2 and 3.
	// 0 used for inactive/new cell. eg. always update.
	MASK_UPDATED = 0b11 << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1 << Shifts::SHIFT_ACTIVE,
	MASK_HUE = 0xF << Shifts::SHIFT_HUE,
	MASK_VALUE = 0xF << Shifts::SHIFT_VALUE,
};

static u32 updated_bit = 0;
inline void update_updated_bit(u64 tick) {
	updated_bit = ((tick % 3) + 1) << Shifts::SHIFT_UPDATED;
}

inline u32 material_idx(const u32 &cell) {
	return cell & Masks::MASK_MATERIAL;
}

inline void set_material_idx(u32 &cell, const u32 material_idx) {
	cell = (cell & ~Masks::MASK_MATERIAL) | material_idx | Masks::MASK_ACTIVE;

	// TODO: Do that during cell update.
	// Handle color.
	// Update flags.
}

inline bool is_updated(const u32 &cell) {
	return (cell & Masks::MASK_UPDATED) == updated_bit;
}

inline void set_updated(u32 &cell) {
	cell = (cell & ~Masks::MASK_UPDATED) | updated_bit;
}

inline bool is_active(const u32 &cell) {
	return cell & Masks::MASK_ACTIVE;
}

// When inactive, set updated bit to 0 (never skip when it return to active).
inline void set_active(u32 &cell, const bool active) {
	if (active) {
		cell |= Masks::MASK_ACTIVE;
	} else {
		cell &= ~Masks::MASK_ACTIVE;
		cell &= ~Masks::MASK_UPDATED;
	}
}

inline void set_hue(u32 &cell, const u32 hue_palette_idx) {
	cell = (cell & ~Masks::MASK_HUE) | (hue_palette_idx << Shifts::SHIFT_HUE);
}

inline void set_value(u32 &cell, const u32 value_palette_idx) {
	cell = (cell & ~Masks::MASK_VALUE) | (value_palette_idx << Shifts::SHIFT_VALUE);
}

} // namespace Cell

#endif
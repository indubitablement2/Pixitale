#ifndef CELL_HPP
#define CELL_HPP

#include "core/variant/binder_common.h"
#include "preludes.h"

namespace Cell {

enum Movement {
	MOVEMENT_SOLID,
	MOVEMENT_POWDER,
	MOVEMENT_LIQUID,
	MOVEMENT_GAS,
};
// VARIANT_ENUM_CAST(Movement);

enum Collision {
	COLLISION_NONE,
	COLLISION_SOLID,
	COLLISION_PLATFORM,
	COLLISION_LIQUID,
};
// VARIANT_ENUM_CAST(Collision);

enum Shifts {
	SHIFT_UPDATED = 12,
	SHIFT_ACTIVE = 14,
	SHIFT_MOVING = 15,
	SHIFT_DIRECTION = 16,
	SHIFT_UNUSED = 17,
	SHIFT_MOVEMENT = 18,
	SHIFT_VALUE = 20,
	SHIFT_COLOR = 24,
};

enum Masks {
	MASK_MATERIAL = 0xFFF,
	// Alternate between 1, 2 and 3.
	// 0 used for inactive/new cell. eg. always update.
	MASK_UPDATED = 0b11 << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1 << Shifts::SHIFT_ACTIVE,
	MASK_MOVING = 1 << Shifts::SHIFT_MOVING,
	// Horizontal direction for moving cells.
	MASK_DIRECTION = 1 << Shifts::SHIFT_DIRECTION,
	// Free real estate!
	MASK_UNUSED = 1 << Shifts::SHIFT_UNUSED,
	// state: solid/powder/liquid/gas
	MASK_MOVEMENT = 0b11 << Shifts::SHIFT_MOVEMENT,
	MASK_VALUE = 0xF << Shifts::SHIFT_VALUE,
	MASK_COLOR = 0xFF << Shifts::SHIFT_COLOR,
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

inline int32_t value(const u32 &cell) {
	return (cell & Masks::MASK_VALUE) >> Shifts::SHIFT_VALUE;
}

inline void set_value(u32 &cell, int32_t value, bool saturate) {
	if (saturate) {
		if (value > 0xFF) {
			value = 0xFF;
		} else if (value < 0) {
			value = 0;
		}
	}

	cell = (cell & ~Masks::MASK_VALUE) | ((u32)value << Shifts::SHIFT_VALUE);
}

} // namespace Cell

#endif
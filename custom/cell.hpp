#ifndef CELL_HPP
#define CELL_HPP

#include "core/typedefs.h"
#include "preludes.h"

namespace Cell {

enum Shifts {
	SHIFT_UPDATED = 12,
	SHIFT_ACTIVE = 13,
	SHIFT_VELOCITY = 14,
	SHIFT_ORTHOGONAL_VELOCITY = 16,
	SHIFT_HUE = 24,
	SHIFT_VALUE = 28,
};

// material_idx: 0..12
// updated: 12
// active: 13
// velocity_type(v/none (0), h+ (1), h- (2), particle (3)): 14..16
// orthogonal_velocity: 16..24
// hue: 24..28
// value: 28..32
// todo
// material_idx: 0..14
// updated: 14
// active: 15
// hue: 16..24
// value: 24..32
// --
// x vel: 32..48
// y vel: 48..64
enum Masks {
	MASK_MATERIAL = 0xFFF,
	MASK_UPDATED = 1 << Shifts::SHIFT_UPDATED,
	MASK_ACTIVE = 1 << Shifts::SHIFT_ACTIVE,
	MASK_VELOCITY_TYPE = 0b11 << Shifts::SHIFT_VELOCITY,
	MASK_ORTHOGONAL_VELOCITY = 0xFF << Shifts::SHIFT_ORTHOGONAL_VELOCITY,
	MASK_VELOCITY = 0x3FF << Shifts::SHIFT_VELOCITY,
	MASK_HUE = 0xF << Shifts::SHIFT_HUE,
	MASK_VALUE = 0xF << Shifts::SHIFT_VALUE,
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
		cell &= ~Masks::MASK_VELOCITY;
	}
}

inline bool is_particle(const u32 cell) {
	return (cell & Masks::MASK_VELOCITY_TYPE) == 3 << Shifts::SHIFT_VELOCITY;
}

inline void set_is_particle(u32 &cell, const bool is_particle) {
	cell &= ~Masks::MASK_VELOCITY;
	if (is_particle) {
		cell |= 3 << Shifts::SHIFT_VELOCITY;
	}
}

// Returns 1, -1 or 0.
inline i32 movement_dir(const u32 cell) {
	u32 velocity_type = (cell & Masks::MASK_VELOCITY_TYPE) >> Shifts::SHIFT_VELOCITY;
	switch (velocity_type) {
		case 1:
			return 1;
		case 2:
			return -1;
		default:
			return 0;
	}
}

// Accepts 1, -1 or 0 (vertical, unknow, none).
inline void set_movement_dir(u32 &cell, const i32 dir) {
	cell = (cell & ~Masks::MASK_VELOCITY_TYPE);
	if (dir > 0) {
		cell |= 1 << Shifts::SHIFT_VELOCITY;
	} else if (dir < 0) {
		cell |= 2 << Shifts::SHIFT_VELOCITY;
	}
}

// Returns `0..256`.
inline u32 orthogonal_velocity_i(const u32 cell) {
	return (cell & Masks::MASK_ORTHOGONAL_VELOCITY) >> Shifts::SHIFT_ORTHOGONAL_VELOCITY;
}

// Returns `0..1`.
inline f32 orthogonal_velocity(const u32 cell) {
	return f32(orthogonal_velocity_i(cell)) / 255.0f;
}

inline void set_orthogonal_velocity(u32 &cell, const f32 orthogonal_velocity) {
	cell &= ~Masks::MASK_ORTHOGONAL_VELOCITY;
	u32 v = u32(CLAMP(orthogonal_velocity, 0.0f, 1.0f) * 255.0f);
	cell |= v << Shifts::SHIFT_ORTHOGONAL_VELOCITY;
}

inline void clear_velocity(u32 &cell) {
	cell &= ~Masks::MASK_VELOCITY;
}

inline void set_hue(u32 &cell, const u32 hue_palette_idx) {
	cell = (cell & ~Masks::MASK_HUE) | (hue_palette_idx << Shifts::SHIFT_HUE);
}

inline void set_value(u32 &cell, const u32 value_palette_idx) {
	cell = (cell & ~Masks::MASK_VALUE) | (value_palette_idx << Shifts::SHIFT_VALUE);
}

inline u32 build_cell(
		const u32 material_idx,
		const u32 updated_mask = 0,
		const bool active = true,
		const u32 hue_palette_idx = 0,
		const u32 value_palette_idx = 0) {
	u32 cell = material_idx;
	set_updated(cell, updated_mask);
	set_active(cell, active);
	set_hue(cell, hue_palette_idx);
	set_value(cell, value_palette_idx);
	return cell;
}

} // namespace Cell

#endif
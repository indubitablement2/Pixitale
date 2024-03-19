#ifndef CELL_MATERIAL_HPP
#define CELL_MATERIAL_HPP

#include "core/object/object.h"
#include "preludes.h"
#include "rng.hpp"

const u32 DISSIPATION_CHANCE = 8388608u; // 0.2%

enum CellCollision {
	CELL_COLLISION_NONE = 0,
	CELL_COLLISION_SOLID = 1,
	CELL_COLLISION_PLATFORM = 2,
	CELL_COLLISION_LIQUID = 4,
};

struct CellReaction {
	// Chance for reaction to happen out of MAX_U32.
	u32 probability;

	// If eq in1 does not change material.
	u32 mat_idx_out1;
	// If eq in2 does not change material.
	u32 mat_idx_out2;

	u16 reaction_id;

	bool callback_swap;

	Callable callback;

	inline bool try_react(Rng &rng) {
		return rng.gen_probability_u32_max(probability);
	}
};

struct CellMaterial {
	// TODO: wind effect when moving vertically.
	// todo: new cell noise

	CellCollision collision = CellCollision::CELL_COLLISION_NONE;

	// Can swap position with less dense cell.
	i32 density = 0;

	// How many vertical movement per step.
	// Negative value moves upwards.
	// For solid, should be 0 (never move vertically).
	i32 vertical_movement = 0;

	// How many horizontally movement per step.
	// Should be >=1.
	i32 horizontal_movement = 1;
	// Chance to spontaneously start moving horizontally when active.
	// Does not keep cell active.
	u32 horizontal_movement_start_chance = 0;
	// Chance to stop moving horizontally.
	u32 horizontal_movement_stop_chance = MAX_U32;

	// Darken new cell, by up to this amount.
	u32 noise_darken_max = 0;

	// Small chance to remove this cell on horizontal movement.
	// This is for top layer of fluid to eventually become inactive,
	// instead of moving back and forth forever.
	bool dissipate_on_horizontal_movement = false;
	// When blocked from moving horizontally, try to reverse direction instead of stopping.
	bool can_reverse_horizontal_movement = false;
	bool can_color = false;

	inline CellMaterial(Object *obj) {
		collision = CellCollision(i32(obj->get("collision_type", nullptr)));

		density = i32(obj->get("density", nullptr));

		vertical_movement = CLAMP(i32(obj->get("vertical_movement", nullptr)), -16, 16);

		horizontal_movement = CLAMP(i32(obj->get("horizontal_movement", nullptr)), 1, 16);
		horizontal_movement_start_chance = u32(CLAMP(f64(obj->get("horizontal_movement_start_chance", nullptr)), 0.0, 1.0) * f64(MAX_U32));
		horizontal_movement_stop_chance = u32(CLAMP(f64(obj->get("horizontal_movement_stop_chance", nullptr)), 0.0, 1.0) * f64(MAX_U32));

		noise_darken_max = MIN(u32(obj->get("noise_darken_max", nullptr)), 63u);

		dissipate_on_horizontal_movement = bool(obj->get("dissipate_on_horizontal_movement", nullptr));

		can_reverse_horizontal_movement = bool(obj->get("can_reverse_horizontal_movement", nullptr));

		can_color = bool(obj->get("can_color", nullptr));
	}
};

#endif // CELL_MATERIAL_H
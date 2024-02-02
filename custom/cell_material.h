#ifndef CELL_MATERIAL_H
#define CELL_MATERIAL_H

#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "preludes.h"
#include "rng.hpp"
#include "vector"
#include <vector>

enum CellCollision {
	CELL_COLLISION_NONE,
	CELL_COLLISION_SOLID,
	CELL_COLLISION_PLATFORM,
	CELL_COLLISION_LIQUID,
};

const u32 CELL_REACTION_PROBABILITY_RANGE = 1 << 16;

struct CellReaction {
	// Chance for reaction to happen out of CELL_REACTION_PROBABILITY_RANGE.
	u16 probability;

	// If eq in1 does not change material.
	u16 mat_idx_out1;
	// If eq in2 does not change material.
	u16 mat_idx_out2;

	u16 reaction_id;

	// todo: optional gdscript function
	void *callback;

	inline bool try_react(Rng &rng) {
		return rng.gen_probability_u32(probability, CELL_REACTION_PROBABILITY_RANGE);
	}
};

class CellMaterial {
private:
	std::vector<u8> values = {};
	u32 values_height = 0;
	u32 values_width = 0;

	// If 0, then no noise.
	u32 max_value_noise = 0;

public:
	// u32 material_idx = 0;

	// Can swap position with less dense cell.
	i32 density = 0;

	CellCollision collision = CellCollision::CELL_COLLISION_NONE;

	// How much velocity gained per step.
	// Negative value moves upwars.
	f32 vertical_acceleration = 1.0f;
	// How many vertical movement per step when at max orthogonal velocity.
	// Fractional part is handled using probability.
	f32 vertical_velocity_max = 4.0f;
	// TODO: wind effect when moving vertically.

	// How much orthogonal velocity gained/lost per step when moving horizontally.
	// Should be set to 1 for fluid and some negative value otherwise.
	f32 horizontal_acceleration;
	// How many horizontal movement per step when at max orthogonal velocity.
	f32 horizontal_velocity_max;
	// Duplicate this cell on vertical movement when moving atop inactive cells.
	// This is for top layer of fluid to eventually fill up and become inactive
	// instead of moving back and forth forever.
	u32 duplicate_on_vertical_movement_probability;

	// When true, cell will fall when not supported by other cells.
	bool can_fall = false;
	bool can_color = false;

	u32 get_value_at(const Vector2i coord, Rng &rng);
	u32 get_hue_at(const Vector2i coord, Rng &rng);

	CellMaterial(Object *obj);
};

#endif // CELL_MATERIAL_H
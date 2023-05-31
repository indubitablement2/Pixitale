#ifndef CELL_MATERIAL_H
#define CELL_MATERIAL_H

#include "cell.hpp"
#include "preludes.h"

struct CellReaction {
	f32 probability;
	// If eq in1 does not change material.
	u32 mat_idx_out1;
	// If eq in2 does not change material.
	u32 mat_idx_out2;
};

class CellMaterial {
	inline static std::vector<CellReaction> reactions = {};

	// Has all reactions with material that have idx >= this material's idx.
	// packed as: start idx(0..20) + len (20..32)
	u32 *reaction_ranges;
	u32 reaction_ranges_len;

public:
	static std::vector<CellMaterial> materials;

	Cell::Movement movement;
	i32 density;

	f32 durability;

	Cell::Collision collision;
	f32 friction;

	bool can_color;

	// on_destroyed: ();

	// higher_reactions is all reactions with material that have idx > this material's idx.
	// Inner vector can be empty (no reactions with this material).
	static void add(
			const Cell::Movement cell_movement,
			const i32 density,
			const f32 durability,
			const Cell::Collision collision,
			const f32 friction,
			const bool can_color,
			const std::vector<std::vector<CellReaction>> higher_reactions);
	static void free_memory();

	static void try_react_between(
			bool &active,
			bool &changed,
			u32 &material_idx,
			const i32 x,
			const i32 y,
			u32 *other_ptr,
			const i32 other_x,
			const i32 other_y,
			u64 &rng);

	void print(u32 material_idx);
};

#endif // CELL_MATERIAL_H
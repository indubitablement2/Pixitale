#ifndef CELL_MATERIAL_H
#define CELL_MATERIAL_H

#include "cell.hpp"
#include "core/io/image.h"
#include "preludes.h"

struct CellReaction {
	f32 probability;
	// If eq in1 does not change material.
	u32 mat_idx_out1;
	// If eq in2 does not change material.
	u32 mat_idx_out2;
};

class CellMaterial {
	// Key is lower material_idx | higher material_idx << 16.
	inline static std::unordered_map<u32, std::vector<CellReaction>> reactions_map = {};

	u8 *values;
	u32 values_height;
	u32 values_width;

public:
	static std::vector<CellMaterial> materials;

	f32 durability;

	Cell::Collision collision;
	f32 friction;

	u32 cell_biome;

	// If 0, then no noise.
	u32 max_value_noise;

	// Can swap position with less dense cell.
	i32 density;
	// 0: no vertical movement.
	// 1: vertical movement down.
	// -1: vertical movement up.
	i32 movement_vertical_step;
	f32 movement_chance;
	bool horizontal_movement;

	bool can_color;

	// higher_reactions is all reactions with material that have idx > this material's idx.
	// Inner vector can be empty (no reactions with this material).
	static void add(
			const i32 density,
			const i32 movement_vertical_step,
			const f32 movement_chance,
			const bool horizontal_movement,
			const f32 durability,
			const Cell::Collision collision,
			const f32 friction,
			const bool can_color,
			const u32 max_value_noise,
			const Ref<Image> values,
			const std::vector<std::vector<CellReaction>> higher_reactions,
			const u32 cell_biome);
	static void free_memory();

	static bool try_react_between(
			u32 *cell_ptr,
			bool &active,
			const u32 material_idx,
			const i32 x,
			const i32 y,
			const i32 other_offset_x,
			const i32 other_offset_y,
			u64 &rng);

	u32 get_value_idx_at(const i32 x, const i32 y, u64 &rng);

	void print(u32 material_idx);
};

#endif // CELL_MATERIAL_H
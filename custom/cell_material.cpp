#include "cell_material.h"

#include "cell.hpp"
#include "core/typedefs.h"
#include "grid.h"
#include "preludes.h"
#include "rng.hpp"

std::vector<CellMaterial> CellMaterial::materials = {};

u32 reations_key(const u32 material_idx_a, const u32 material_idx_b, bool &swap) {
	if (material_idx_a <= material_idx_b) {
		swap = false;
		return material_idx_a | (material_idx_b << 16);
	} else {
		swap = true;
		return material_idx_b | (material_idx_a << 16);
	}
}

void CellMaterial::add(
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
		const u32 cell_biome) {
	CellMaterial cell_material = CellMaterial();
	cell_material.density = density;
	cell_material.horizontal_movement = horizontal_movement;
	cell_material.movement_vertical_step = movement_vertical_step;
	cell_material.movement_chance = movement_chance;
	cell_material.durability = durability;
	cell_material.collision = collision;
	cell_material.friction = friction;
	cell_material.can_color = can_color;
	cell_material.cell_biome = cell_biome;

	// Add values noise.
	cell_material.max_value_noise = max_value_noise + 1;
	cell_material.max_value_noise = CLAMP(cell_material.max_value_noise, 1u, 16u);
	if (cell_material.max_value_noise == 1) {
		cell_material.max_value_noise = 0;
	}

	// Add values image.
	if (values.is_valid()) {
		cell_material.values_width = values->get_width();
		cell_material.values_height = values->get_height();
		cell_material.values = new u8[cell_material.values_width * cell_material.values_height];
		for (u32 y = 0; y < cell_material.values_height; y++) {
			for (u32 x = 0; x < cell_material.values_width; x++) {
				f32 v = values->get_pixel(x, y).r;

				// Reverse value. 0 is brightest, 15 is darkest.
				v = 1.0f - v;

				v *= 16.0f;
				v = CLAMP(v, 0.0f, 15.0f);

				cell_material.values[y * cell_material.values_width + x] = u8(v);
			}
		}
	} else {
		cell_material.values_width = 0;
		cell_material.values_height = 0;
		cell_material.values = nullptr;
	}

	u32 material_idx = materials.size();
	materials.push_back(cell_material);

	for (u32 i = 0; i < higher_reactions.size(); i++) {
		u32 other_material_idx = material_idx + i;

		bool swap;
		u32 reactions_key = reations_key(
				material_idx,
				other_material_idx,
				swap);
		TEST_ASSERT(!swap, "should not swap");

		reactions_map[reactions_key] = higher_reactions[i];
	}
}

void CellMaterial::free_memory() {
	reactions_map = {};
	materials = {};
}

bool CellMaterial::try_react_between(
		u32 *cell_ptr,
		bool &active,
		const u32 material_idx,
		const i32 x,
		const i32 y,
		const i32 other_offset_x,
		const i32 other_offset_y,
		u64 &rng) {
	u32 *other_ptr = cell_ptr + (other_offset_x + other_offset_y * Grid::width);
	u32 other_material_idx = Cell::material_idx(*other_ptr);

	bool swap;
	u32 reactions_key = reations_key(
			material_idx,
			other_material_idx,
			swap);

	std::vector<CellReaction> &reactions = reactions_map[reactions_key];

	for (u32 i = 0; i < reactions.size(); i++) {
		CellReaction &reaction = reactions[i];

		if (Rng::gen_probability(rng, reaction.probability)) {
			u32 out1, out2;
			if (swap) {
				out1 = reaction.mat_idx_out2;
				out2 = reaction.mat_idx_out1;
			} else {
				out1 = reaction.mat_idx_out1;
				out2 = reaction.mat_idx_out2;
			}

			if (out1 != material_idx) {
				*cell_ptr = out1;
			}
			if (out2 != other_material_idx) {
				*other_ptr = out2;
			}

			Cell::set_updated(*other_ptr);
			Cell::set_updated(*cell_ptr);

			Grid::activate_neighbors(x, y, cell_ptr);
			Grid::activate_neighbors(x + other_offset_x, y + other_offset_y, other_ptr);

			return true;
		}
	}

	if (!reactions.empty()) {
		active = true;
	}

	return false;
}

u32 CellMaterial::get_value_idx_at(const i32 x, const i32 y, u64 &rng) {
	u32 value = 0;

	if (values != nullptr) {
		value = (u32)values[((u32)x % values_width) + ((u32)y % values_height) * values_width];
	}

	if (max_value_noise) {
		value += Rng::gen_range_u32(rng, 0, max_value_noise);
		value = MIN(value, 15u);
	}

	return value;
}

void CellMaterial::print(u32 material_idx) {
	print_line("-----------", material_idx, "-----------");

	print_line("durability ", durability);

	print_line("cell_collision ", collision);
	print_line("friction ", friction);

	print_line("cell_biome ", cell_biome);

	print_line("can_change_hue ", can_color);
	print_line("values_width ", values_width);
	print_line("values_height ", values_height);
	print_line("max_value_noise ", max_value_noise);

	print_line("density ", density);
	print_line("movement_vertical_step ", movement_vertical_step);
	print_line("movement_chance ", movement_chance);
	print_line("horizontal_movement ", horizontal_movement);
}
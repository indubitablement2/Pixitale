#include "cell_material.h"

#include "cell.hpp"
#include "core/typedefs.h"
#include "grid.h"
#include "rng.hpp"

std::vector<CellMaterial> CellMaterial::materials = {};

void CellMaterial::add(
		const Cell::Movement cell_movement,
		const i32 density,
		const f32 durability,
		const Cell::Collision collision,
		const f32 friction,
		const bool can_color,
		const u32 min_value_noise,
		const u32 max_value_noise,
		const Ref<Image> values,
		const std::vector<std::vector<CellReaction>> higher_reactions) {
	CellMaterial cell_material = CellMaterial();
	cell_material.movement = cell_movement;
	cell_material.density = density;
	cell_material.durability = durability;
	cell_material.collision = collision;
	cell_material.friction = friction;
	cell_material.can_color = can_color;

	// Add values noise.
	cell_material.min_value_noise = min_value_noise;
	cell_material.max_value_noise = max_value_noise;
	cell_material.max_value_noise = CLAMP(cell_material.max_value_noise, 0u, 16u);
	cell_material.min_value_noise = CLAMP(
			cell_material.min_value_noise,
			0u,
			cell_material.max_value_noise);

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

	cell_material.reaction_ranges = nullptr;
	cell_material.reaction_ranges_len = 0;
	for (u32 i = 0; i < higher_reactions.size(); i++) {
		if (!higher_reactions[i].empty()) {
			cell_material.reaction_ranges_len = i + 1;
		}
	}
	if (cell_material.reaction_ranges_len > 0) {
		cell_material.reaction_ranges = new u32[cell_material.reaction_ranges_len];

		for (u32 i = 0; i < cell_material.reaction_ranges_len; i++) {
			if (higher_reactions[i].empty()) {
				cell_material.reaction_ranges[i] = 0;
				continue;
			}

			// Pack start and len into one u32.
			cell_material.reaction_ranges[i] =
					cell_material.reactions.size() | (higher_reactions[i].size() << 20);

			for (u32 j = 0; j < higher_reactions[i].size(); j++) {
				cell_material.reactions.push_back(higher_reactions[i][j]);
			}
		}
	}

	materials.push_back(cell_material);
}

void CellMaterial::free_memory() {
	for (u32 i = 0; i < materials.size(); i++) {
		if (materials[i].reaction_ranges != nullptr) {
			delete[] materials[i].reaction_ranges;
		}
		if (materials[i].values != nullptr) {
			delete[] materials[i].values;
		}
	}
	materials = {};
}

void CellMaterial::try_react_between(
		bool &active,
		bool &changed,
		u32 &material_idx,
		const i32 x,
		const i32 y,
		u32 *other_ptr,
		const i32 other_x,
		const i32 other_y,
		u64 &rng) {
	u32 other_material_idx = Cell::material_idx(*other_ptr);

	bool swap;
	u32 reaction_range_idx;
	CellMaterial *mat;
	if (material_idx > other_material_idx) {
		swap = true;
		reaction_range_idx = material_idx - other_material_idx;
		mat = materials.data() + other_material_idx;
	} else {
		swap = false;
		reaction_range_idx = other_material_idx - material_idx;
		mat = materials.data() + material_idx;
	}

	if (mat->reaction_ranges_len >= reaction_range_idx) {
		return;
	}
	u32 packed_reaction_range = mat->reaction_ranges[reaction_range_idx];

	if (packed_reaction_range == 0) {
		return;
	}

	active = true;

	u32 reaction_range_start = packed_reaction_range & 0xFFFFF;
	u32 reaction_range_end = reaction_range_start + (packed_reaction_range >> 20);

	for (u32 i = reaction_range_start; i < reaction_range_end; i++) {
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
				material_idx = out1;
				changed = true;
			}

			if (out2 != other_material_idx) {
				Cell::set_material_idx(*other_ptr, out2);
				Grid::activate_neighbors(other_x, other_y, other_ptr);
			}

			return;
		}
	}
}

u32 CellMaterial::get_value_idx_at(const i32 x, const i32 y, u64 &rng) {
	u32 value = 0;

	if (max_value_noise) {
		value = Rng::gen_range_u32(rng, min_value_noise, max_value_noise);
	}

	if (values != nullptr) {
		value += (u32)values[((u32)x % values_width) + ((u32)y % values_height) * values_width];
		value = MIN(value, 15u);
	}

	return value;
}

void CellMaterial::print(u32 material_idx) {
	print_line("-----------", material_idx, "-----------");

	print_line("cell_movement ", movement);
	print_line("density ", density);
	print_line("durability ", durability);
	print_line("cell_collision ", collision);
	print_line("friction ", friction);

	print_line("can_change_hue ", can_color);

	print_line("values_width ", values_width);
	print_line("values_height ", values_height);

	print_line("min_value_noise ", min_value_noise);
	print_line("max_value_noise ", max_value_noise);

	print_line("reaction_ranges_len: ", reaction_ranges_len);

	u32 used_reaction_range = 0;

	for (u32 other_material_idx_offset = 0; other_material_idx_offset < reaction_ranges_len; other_material_idx_offset++) {
		u32 reaction_range = *(reaction_ranges + other_material_idx_offset);

		if (reaction_range == 0) {
			continue;
		}

		used_reaction_range += 1;

		u32 other_material_idx = material_idx + other_material_idx_offset;

		u32 reaction_start = reaction_range & 0xFFFFF;
		u32 reaction_len = reaction_range >> 20;
		u32 reaction_end = reaction_start + reaction_len;

		print_line("       reaction with ", other_material_idx);
		print_line("       num reactions ", reaction_len);

		for (u32 reaction_idx = reaction_start; reaction_idx < reaction_end; reaction_idx++) {
			CellReaction *reaction = reactions.data() + reaction_idx;
			print_line("          probability ", reaction->probability);
			print_line("          in1 ", material_idx, " -> out1 ", reaction->mat_idx_out1);
			print_line("          in2 ", other_material_idx, " -> out2 ", reaction->mat_idx_out2);
		}
	}

	print_line(
			"used_reaction_range: ",
			used_reaction_range,
			" / ",
			reaction_ranges_len,
			" (",
			reaction_ranges_len - used_reaction_range,
			" is wasted)");
}
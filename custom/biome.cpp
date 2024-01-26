// #include "biome.h"

// #include "cell.hpp"
// #include "cell_material.h"
// #include "core/error/error_macros.h"
// #include "core/object/class_db.h"
// #include "grid.h"
// #include "preludes.h"
// #include <cmath>

// void GridBiomeScanner::_bind_methods() {
// 	ClassDB::bind_method(
// 			D_METHOD("scan"),
// 			&GridBiomeScanner::scan);
// 	ClassDB::bind_method(
// 			D_METHOD("get_current_biome"),
// 			&GridBiomeScanner::get_current_biome);
// }

// bool GridBiomeScanner::scan() {
// 	if (biomes.empty()) {
// 		return false;
// 	}

// 	for (u32 i = 0; i < cell_biome_counts.size(); i++) {
// 		cell_biome_counts[i] = 0;
// 	}

// 	Vector2 pos = get_global_position();

// 	const i32 half_x_size = 160;
// 	const i32 half_y_size = 160;
// 	i32 x_start = i32(pos.x) - half_x_size;
// 	i32 x_end = x_start + half_x_size * 2;
// 	i32 y_start = i32(pos.y) - half_y_size;
// 	i32 y_end = y_start + half_y_size * 2;

// 	for (i32 y = y_start; y < y_end; y += 4) {
// 		for (i32 x = x_start; x < x_end; x += 1) {
// 			u32 material_idx = Cell::material_idx(Grid::get_cell_checked(x, y));
// 			u32 cell_biome = CellMaterial::materials[material_idx].biome_contribution;

// 			cell_biome_counts[cell_biome] += 1;
// 		}
// 	}

// 	f32 depth = pos.y / f32(Grid::height);
// 	f32 distance_from_center = abs((pos.x / f32(Grid::width)) * 2.0f - 1.0f);

// 	u32 new_biome_idx = 0;
// 	while (new_biome_idx < biomes.size() - 1) {
// 		Biome &biome = biomes[new_biome_idx];

// 		if (depth > biome.min_depth &&
// 				distance_from_center > biome.min_distance_from_center &&
// 				cell_biome_counts[new_biome_idx] >= biome.min_cell_biome_count) {
// 			break;
// 		}

// 		new_biome_idx += 1;
// 	}

// 	if (new_biome_idx != current_biome_idx) {
// 		current_biome_idx = new_biome_idx;
// 		return true;
// 	}

// 	return false;
// }

// u32 GridBiomeScanner::get_current_biome() {
// 	return current_biome_idx;
// }

// void GridBiomeScanner::set_biomes(std::vector<Biome> new_biomes) {
// 	ERR_FAIL_COND_MSG(new_biomes.empty(), "Biomes must not be empty.");

// 	biomes = new_biomes;
// 	cell_biome_counts.resize(biomes.size());
// }
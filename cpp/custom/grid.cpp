#include "grid.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/typedefs.h"
#include "core/variant/array.h"
#include "preludes.h"

#include "BS_thread_pool.hpp"
#include "biome.h"
#include "cell.hpp"
#include "cell_material.h"
#include "chunk.hpp"
#include "rng.hpp"

inline static BS::thread_pool tp = BS::thread_pool();

namespace Step {

i32 get_movement_dir(u32 &cell, u64 &rng) {
	u32 n = Cell::movement_dir(cell);

	if (n == 0) {
		if (Rng::gen_bool(rng)) {
			n = 1;
		} else {
			n = 3;
		}
		Cell::set_movement_dir(cell, n);
	}

	return (i32)n - 2;
}

void flip_movement_dir(u32 &cell, i32 &dir) {
	dir = -dir;
	Cell::set_movement_dir(cell, (u32)(dir + 2));
}

void swap_cells(
		u32 *cell_ptr,
		i32 x,
		i32 y,
		u32 *other_ptr,
		i32 other_x,
		i32 other_y) {
	u32 cell = *cell_ptr;
	*cell_ptr = *other_ptr;
	*other_ptr = cell;

	Cell::set_updated(*cell_ptr);
	Cell::set_updated(*other_ptr);

	Grid::activate_neighbors(x, y, cell_ptr);
	Grid::activate_neighbors(other_x, other_y, other_ptr);
}

bool try_swap_h(
		u32 *cell_ptr,
		i32 x,
		i32 y,
		CellMaterial &cell_material,
		i32 dir,
		u64 &rng) {
	u32 *other_ptr = cell_ptr + dir;
	CellMaterial &other_material = CellMaterial::materials[Cell::material_idx(*other_ptr)];

	if (cell_material.density > other_material.density) {
		// Chance to copy cells.
		const u32 HORIZONTAL_MOVEMENT_DUPLICATE_CHANCE = 2097152;
		if (Rng::gen_u32(rng) < HORIZONTAL_MOVEMENT_DUPLICATE_CHANCE) {
			Cell::set_updated(*cell_ptr);
			*other_ptr = *cell_ptr;

			Grid::activate_neighbors(x, y, cell_ptr);
			Grid::activate_neighbors(x + dir, y, other_ptr);

			return true;
		}

		swap_cells(
				cell_ptr,
				x,
				y,
				other_ptr,
				x + dir,
				y);

		return true;
	} else {
		return false;
	}
}

bool try_swap_v(
		u32 *cell_ptr,
		i32 x,
		i32 y,
		CellMaterial &cell_material,
		i32 x_offset,
		i32 y_offset) {
	u32 *other_ptr = cell_ptr + x_offset + y_offset * Grid::width;
	CellMaterial &other_material = CellMaterial::materials[Cell::material_idx(*other_ptr)];

	if (cell_material.density > other_material.density) {
		swap_cells(
				cell_ptr,
				x,
				y,
				other_ptr,
				x + x_offset,
				y + y_offset);

		return true;
	} else {
		return false;
	}
}

void step_cell(
		i32 x,
		i32 y,
		u64 &rng) {
	TEST_ASSERT(x < Grid::width - 32, "x too high");
	TEST_ASSERT(x > 31, "x too low");
	TEST_ASSERT(y < Grid::height - 32, "y too high");
	TEST_ASSERT(y > 31, "y too low");

	u32 *cell_ptr = Grid::cells + x + y * Grid::width;

	if (!Cell::is_active(*cell_ptr)) {
		return;
	}
	if (Cell::is_updated(*cell_ptr)) {
		Chunk::activate_point(x, y);
		return;
	}

	u32 cell_material_idx = Cell::material_idx(*cell_ptr);
	bool active = false;

	// Reactions
	// x x x
	// . o x
	// . . .
	if (CellMaterial::try_react_between(
				cell_ptr,
				active,
				cell_material_idx,
				x,
				y,
				1,
				0,
				rng)) {
		return;
	}
	if (CellMaterial::try_react_between(
				cell_ptr,
				active,
				cell_material_idx,
				x,
				y,
				-1,
				-1,
				rng)) {
		return;
	}
	if (CellMaterial::try_react_between(
				cell_ptr,
				active,
				cell_material_idx,
				x,
				y,
				0,
				-1,
				rng)) {
		return;
	}
	if (CellMaterial::try_react_between(
				cell_ptr,
				active,
				cell_material_idx,
				x,
				y,
				1,
				-1,
				rng)) {
		return;
	}

	// Movement

	CellMaterial &cell_material = CellMaterial::materials[cell_material_idx];

	if (cell_material.movement_vertical_step != 0) {
		// Vertical movement not affected by slow movement.
		if (try_swap_v(
					cell_ptr,
					x,
					y,
					cell_material,
					0,
					cell_material.movement_vertical_step)) {
			return;
		}

		if (Rng::gen_probability(rng, cell_material.movement_chance)) {
			// Diagonal movement.
			i32 dir;
			if (Rng::gen_bool(rng)) {
				dir = 1;
			} else {
				dir = -1;
			}
			if (try_swap_v(
						cell_ptr,
						x,
						y,
						cell_material,
						dir,
						cell_material.movement_vertical_step)) {
				return;
			}
			if (try_swap_v(
						cell_ptr,
						x,
						y,
						cell_material,
						-dir,
						cell_material.movement_vertical_step)) {
				return;
			}

			// Horizontal movement.
			if (cell_material.horizontal_movement) {
				dir = get_movement_dir(*cell_ptr, rng);
				if (try_swap_h(
							cell_ptr,
							x,
							y,
							cell_material,
							dir,
							rng)) {
					return;
				}
				flip_movement_dir(*cell_ptr, dir);
				if (try_swap_h(
							cell_ptr,
							x,
							y,
							cell_material,
							dir,
							rng)) {
					return;
				}
			}
		} else {
			active = true;
		}
	}

	Cell::set_updated(*cell_ptr);
	if (active) {
		Cell::set_active(*cell_ptr, true);
		Chunk::activate_point(x, y);
	} else {
		Cell::set_active(*cell_ptr, false);
	}
}

void step_chunk(
		u64 chunk,
		i32 x,
		i32 y,
		u64 &rng) {
	if (chunk == 0) {
		return;
	}

	u32 rows = Chunk::get_rows(chunk);
	auto rect = Chunk::active_rect(chunk);

	// Alternate iteration between left and right.
	i32 x_start;
	i32 x_end;
	i32 x_step;
	if ((Grid::tick & 1) == 0) {
		x_start = rect.x_start;
		x_end = rect.x_end;
		x_step = 1;
	} else {
		x_start = rect.x_end - 1;
		x_end = rect.x_start - 1;
		x_step = -1;
	}

	// Iterate over each cell in the chunk.
	for (i32 local_y = rect.y_start; local_y < rect.y_end; local_y++) {
		if ((rows & (1u << local_y)) == 0) {
			continue;
		}

		i32 local_x = x_start;
		while (local_x != x_end) {
			step_cell(x + local_x, y + local_y, rng);

			local_x += x_step;
		}
	}
}

void step_row(i32 row_idx) {
	bool is_row_active = false;

	u64 rng = ((u64)row_idx + (u64)Grid::tick) * 6364136223846792969uLL;

	u64 *chunk_ptr = Grid::chunks + row_idx * Grid::chunks_width;
	u64 *chunk_ptr_end = chunk_ptr + Grid::chunks_width - 2;
	i32 x = 0;
	i32 y = row_idx * 32;

	// Iterate over each chunk left to right.
	while (chunk_ptr < chunk_ptr_end) {
		chunk_ptr += 1;
		x += 32;

		u64 chunk = *chunk_ptr;
		*chunk_ptr = 0;

		step_chunk(chunk, x, y, rng);

		if (*chunk_ptr != 0) {
			is_row_active = true;
		}
	}

	if (is_row_active) {
		Grid::active_rows[row_idx - 1] = 1;
		Grid::active_rows[row_idx] = 1;
		Grid::active_rows[row_idx + 1] = 1;
	} else {
		Grid::active_rows[row_idx] = 0;
	}
}

void copy_border_chunk(
		i32 x_start,
		i32 y_start) {
	for (i32 y = y_start; y < y_start + 32; y++) {
		for (i32 x = 0; x < 32; x++) {
			i32 grid_x = x_start + x;
			Grid::cells[y * Grid::width + grid_x] = Grid::border_cells[y * 32 + x];
		}
	}
}

void copy_stripe_h(i32 x_start, i32 y, u32 expected_material, u32 copy_cell) {
	for (i32 i = 0; i < 32; i++) {
		i32 x = x_start + i;

		u32 *cell_ptr = Grid::cells + y * Grid::width + x;

		if (Cell::material_idx(*cell_ptr) != expected_material) {
			*cell_ptr = copy_cell;
			Grid::activate_neighbors(x, y, cell_ptr);
		}
	}
}

void copy_stripe_v(i32 x, i32 y_start, i32 x_border) {
	for (i32 i = 0; i < 32; i++) {
		i32 y = y_start + i;

		u32 copy_cell = Grid::border_cells[y * 32 + x_border];
		u32 expected_material = Cell::material_idx(copy_cell);

		u32 *cell_ptr = Grid::cells + y * Grid::width + x;

		if (Cell::material_idx(*cell_ptr) != expected_material) {
			*cell_ptr = copy_cell;
			Grid::activate_neighbors(x, y, cell_ptr);
		}
	}
}

void step_borders() {
	// Top
	if (Grid::active_rows[0] != 0) {
		for (i32 chunk_idx = 0; chunk_idx < Grid::chunks_width; chunk_idx++) {
			if (Grid::chunks[chunk_idx] == 0) {
				continue;
			}

			copy_border_chunk(chunk_idx * 32, 0);
			Grid::chunks[chunk_idx] = 0;
		}

		Grid::active_rows[0] = 0;
	}
	// Bot
	if (Grid::active_rows[Grid::chunks_height - 1] != 0) {
		for (i32 chunk_idx = 0; chunk_idx < Grid::chunks_width; chunk_idx++) {
			if (Grid::chunks[chunk_idx] == 0) {
				continue;
			}

			copy_border_chunk(chunk_idx * 32, 0);
			Grid::chunks[chunk_idx] = 0;
		}

		Grid::active_rows[Grid::chunks_height - 1] = 0;
	}
	// Left & right
	for (i32 chunk_y = 1; chunk_y < Grid::chunks_height - 1; chunk_y++) {
		if (Grid::chunks[chunk_y * Grid::chunks_width] != 0) {
			copy_border_chunk(0, chunk_y * 32);
			Grid::chunks[chunk_y * Grid::chunks_width] = 0;
		}
		if (Grid::chunks[chunk_y * Grid::chunks_width + Grid::chunks_width - 1] != 0) {
			copy_border_chunk(Grid::width - 32, chunk_y * 32);
			Grid::chunks[chunk_y * Grid::chunks_width + Grid::chunks_width - 1] = 0;
		}
	}

	// Apply 1 cell border on the active part of the grid as well.
	// Top
	u32 copy_cell = Grid::get_cell_checked(0, 0);
	u32 expected_material = Cell::material_idx(copy_cell);
	i32 chunk_x = 1;
	i32 chunk_y = 1;
	i32 y = 32;
	while (chunk_x < Grid::chunks_width - 1) {
		if (Grid::chunks[chunk_y * Grid::chunks_width + chunk_x] != 0) {
			copy_stripe_h(chunk_x * 32, y, expected_material, copy_cell);
		}

		chunk_x += 1;
	}
	// Bot
	copy_cell = Grid::get_cell_checked(0, Grid::height);
	expected_material = Cell::material_idx(copy_cell);
	chunk_x = 1;
	chunk_y = Grid::chunks_height - 2;
	y = Grid::height - 32 - 1;
	while (chunk_x < Grid::chunks_width - 1) {
		if (Grid::chunks[chunk_y * Grid::chunks_width + chunk_x] != 0) {
			copy_stripe_h(chunk_x * 32, y, expected_material, copy_cell);
		}

		chunk_x += 1;
	}
	// Left
	chunk_x = 1;
	chunk_y = 2;
	i32 x = 32;
	while (chunk_y < Grid::chunks_height - 2) {
		if (Grid::chunks[chunk_y * Grid::chunks_width + chunk_x] != 0) {
			copy_stripe_v(x, chunk_y * 32, 0);
		}

		chunk_y += 1;
	}
	// Right
	chunk_x = Grid::chunks_width - 2;
	chunk_y = 2;
	x = Grid::width - 32 - 1;
	while (chunk_y < Grid::chunks_height - 2) {
		if (Grid::chunks[chunk_y * Grid::chunks_width + chunk_x] != 0) {
			copy_stripe_v(x, chunk_y * 32, 31);
		}

		chunk_y += 1;
	}
}

} // namespace Step

void Grid::_bind_methods() {
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("delete_grid"),
			&Grid::delete_grid);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("new_empty", "wish_width", "wish_height"),
			&Grid::new_empty);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_size"),
			&Grid::get_size);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_size_chunk"),
			&Grid::get_size_chunk);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_data", "image_size", "rect"),
			&Grid::get_cell_data);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_cell_rect", "rect", "cell_material_idx"),
			&Grid::set_cell_rect);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_cell", "position", "cell_material_idx"),
			&Grid::set_cell);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD(
					"set_cell_color",
					"position",
					"hue_palette_idx",
					"value_palette_idx"),
			&Grid::set_cell_color);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("take_border_cells"),
			&Grid::take_border_cells);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("post_generation_pass"),
			&Grid::post_generation_pass);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("step"),
			&Grid::step);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_tick"),
			&Grid::get_tick);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_material_idx", "position"),
			&Grid::get_cell_material_idx);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD(
					"add_material",
					"density",
					"movement_vertical_step",
					"movement_chance",
					"horizontal_movement",
					"durability",
					"cell_collision",
					"friction",
					"can_color",
					"max_value_noise",
					"values",
					"reactions",
					"cell_biome"),
			&Grid::add_material);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_biomes", "biomes"),
			&Grid::set_biomes);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_seed"),
			&Grid::get_seed);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_seed", "seed"),
			&Grid::set_seed);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("is_chunk_active", "chunk_position"),
			&Grid::is_chunk_active);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("free_memory"),
			&Grid::free_memory);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("print_materials"),
			&Grid::print_materials);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("run_tests"),
			&Grid::run_tests);

	// ADD_GROUP("Test group", "group_");
	// ADD_SUBGROUP("Test subgroup", "group_subgroup_");

	ClassDB::bind_integer_constant("Grid", "", "PALETTE_IDX_MAX", 16);

	using namespace Cell;

	BIND_ENUM_CONSTANT(COLLISION_NONE);
	BIND_ENUM_CONSTANT(COLLISION_SOLID);
	BIND_ENUM_CONSTANT(COLLISION_PLATFORM);
	BIND_ENUM_CONSTANT(COLLISION_LIQUID);
}

void Grid::delete_grid() {
	if (cells != nullptr) {
		print_line("Deleting grid");

		delete[] cells;
		cells = nullptr;
		width = 0;
		height = 0;

		delete[] border_cells;
		border_cells = nullptr;

		delete[] chunks;
		chunks = nullptr;
		chunks_width = 0;
		chunks_height = 0;

		delete[] active_rows;
		active_rows = nullptr;
	}
}

void Grid::new_empty(i32 wish_width, i32 wish_height) {
	delete_grid();

	chunks_width = CLAMP(wish_width / 32, 8, 2048);
	// Make sure that width is a multiple of 64/8(8).
	// This is to avoid mutably sharing cache lines between threads.
	// Chunks are only 8 bytes.
	while (chunks_width % 8 != 0) {
		chunks_width++;
	}

	chunks_height = CLAMP(wish_height / 32, 3, 2048);
	chunks = new u64[chunks_width * chunks_height];
	// Set all chunk to active.
	for (i32 i = 0; i < chunks_width * chunks_height; i++) {
		chunks[i] = ~0uLL;
	}

	width = chunks_width * 32;
	height = chunks_height * 32;
	cells = new u32[width * height];
	// Set all cells to empty and active.
	for (i32 i = 0; i < width * height; i++) {
		cells[i] = Cell::Masks::MASK_ACTIVE;
	}

	border_cells = new u32[height * 32];
	// Set all border cells to empty.
	for (i32 i = 0; i < height * 32; i++) {
		border_cells[i] = 0;
	}

	active_rows = new u64[chunks_height];
	// Set all rows to active.
	for (i32 i = 0; i < chunks_height; i++) {
		active_rows[i] = 1;
	}
}

Vector2i Grid::get_size() {
	return Vector2i(width, height);
}

Vector2i Grid::get_size_chunk() {
	return Vector2i(chunks_width, chunks_height);
}

Ref<Image> Grid::get_cell_data(Vector2i image_size, Rect2i rect) {
	// TODO: Try to use set_cell instead to avoid creating a new buffer.
	auto image_data = PackedByteArray();
	image_data.resize(image_size.x * image_size.y * 4);
	auto image_buffer = reinterpret_cast<u32 *>(image_data.ptrw());

	// TODO: Could be optimized by handling oob separately.
	for (i32 img_y = 0; img_y < MIN(image_size.y, rect.size.y); img_y++) {
		const i32 cell_y = rect.position.y + img_y;

		for (i32 img_x = 0; img_x < MIN(image_size.x, rect.size.x); img_x++) {
			const i32 cell_x = rect.position.x + img_x;

			image_buffer[img_y * image_size.x + img_x] = get_cell_checked(cell_x, cell_y);
		}
	}

	return Image::create_from_data(
			image_size.x,
			image_size.y,
			false,
			Image::FORMAT_RF,
			image_data);
}

u32 Grid::get_cell_checked(i32 x, i32 y) {
	if (cells == nullptr) {
		return 0;
	} else if (y < 0) {
		return border_cells[0];
	} else if (y >= height) {
		return border_cells[(height - 1) * 32];
	} else if (x < 0 || x >= width) {
		return border_cells[y * 32 + (x & 31)];
	}

	return cells[y * width + x];
}

void Grid::activate_neighbors(i32 x, i32 y, u32 *cell_ptr) {
	TEST_ASSERT(x >= 1, "x < 1");
	TEST_ASSERT(y >= 1, "y < 1");
	TEST_ASSERT(x < width - 1, "x >= width - 1");
	TEST_ASSERT(y < height - 1, "y >= height - 1");

	TEST_ASSERT((cells + y * width + x) == cell_ptr, "wrong coords");

	// Activate chunks.
	Chunk::activate_point(x - 1, y - 1);
	Chunk::activate_point(x, y - 1);
	Chunk::activate_point(x + 1, y - 1);

	Chunk::activate_point(x - 1, y);
	// Chunk::activate_point(x, y);
	Chunk::activate_point(x + 1, y);

	Chunk::activate_point(x - 1, y + 1);
	Chunk::activate_point(x, y + 1);
	Chunk::activate_point(x + 1, y + 1);

	// Activate cells.
	Cell::set_active(cell_ptr[-width - 1], true);
	Cell::set_active(cell_ptr[-width], true);
	Cell::set_active(cell_ptr[-width + 1], true);

	Cell::set_active(cell_ptr[-1], true);
	Cell::set_active(cell_ptr[0], true);
	Cell::set_active(cell_ptr[1], true);

	Cell::set_active(cell_ptr[width - 1], true);
	Cell::set_active(cell_ptr[width], true);
	Cell::set_active(cell_ptr[width + 1], true);
}

void Grid::set_cell_rect(Rect2i rect, u32 cell_material_idx) {
	rect = rect.intersection(Rect2i(32, 32, width - 64, height - 64));
	if (rect.size.x <= 0 || rect.size.y <= 0) {
		// Empty rect.
		return;
	}

	CellMaterial &cell_material = CellMaterial::materials[cell_material_idx];
	u32 cell = cell_material_idx;

	for (i32 y = rect.position.y; y < rect.get_end().y; y++) {
		for (i32 x = rect.position.x; x < rect.get_end().x; x++) {
			auto cell_ptr = cells + y * width + x;

			TEST_ASSERT(cell_ptr >= cells, "ptr out of bounds (< cells)");
			TEST_ASSERT(cell_ptr < cells + width * height, "ptr out of bounds (>= cells + width * height)");

			Cell::set_value(cell, cell_material.get_value_idx_at(x, y, rng));

			*cell_ptr = cell;
		}
	}

	for (i32 y = rect.position.y - 1; y < rect.get_end().y + 1; y++) {
		for (i32 x = rect.position.x - 1; x < rect.get_end().x + 1; x++) {
			auto cell_ptr = cells + y * width + x;

			TEST_ASSERT(cell_ptr >= cells, "ptr out of bounds (< cells)");
			TEST_ASSERT(cell_ptr < cells + width * height, "ptr out of bounds (>= cells + width * height)");

			Cell::set_active(*cell_ptr, true);
			Chunk::activate_point(x, y);
		}
	}

	// Activate rows.
	for (i32 y = rect.position.y - 1; y < rect.get_end().y; y += 32) {
		active_rows[y >> 5] |= 1uLL;
	}
	active_rows[(rect.get_end().y + 1) >> 5] = 1uLL;
}

void Grid::set_cell(Vector2i position, u32 cell_material_idx) {
	if (position.x < 32 || position.x >= width - 32 || position.y < 32 || position.y >= height - 32) {
		return;
	}

	CellMaterial &cell_material = CellMaterial::materials[cell_material_idx];
	auto cell_ptr = cells + position.y * width + position.x;
	*cell_ptr = cell_material_idx;
	Cell::set_value(
			*cell_ptr,
			cell_material.get_value_idx_at(position.x, position.y, rng));

	activate_neighbors(position.x, position.y, cell_ptr);
	active_rows[(position.y) >> 5] = 1uLL;
}

void Grid::set_cell_color(Vector2i position, u32 hue_palette_idx, u32 value_palette_idx) {
	if (position.x < 32 || position.x >= width - 32 || position.y < 32 || position.y >= height - 32) {
		return;
	}

	u32 *cell_ptr = cells + position.y * width + position.x;
	u32 cell = *cell_ptr;

	u32 material_idx = Cell::material_idx(cell);

	if (!CellMaterial::materials[material_idx].can_color) {
		hue_palette_idx = 0;
		value_palette_idx = 0;
	}

	Cell::set_hue(cell, hue_palette_idx);
	Cell::set_value(cell, value_palette_idx);

	*cell_ptr = cell;
}

void Grid::take_border_cells() {
	for (i32 y = 0; y < height; y++) {
		for (i32 x = 0; x < 32; x++) {
			u32 cell = cells[y * width + x];
			// Set border cells to inactive.
			Cell::set_active(cell, false);

			border_cells[y * 32 + x] = cell;
		}
	}
}

void Grid::post_generation_pass() {
	// Set everything to active.
	for (i32 i = 0; i < width * height; i++) {
		Cell::set_active(cells[i], true);
	}
	for (i32 i = 0; i < chunks_width * chunks_height; i++) {
		chunks[i] = MAX_U64;
	}
	for (i32 i = 0; i < chunks_height; i++) {
		active_rows[i] = 1;
	}

	for (i32 y = 0; y < height; y++) {
		for (i32 x = 0; x < width; x++) {
			u32 cell = cells[y * width + x];
			u32 material_idx = Cell::material_idx(cell);
			u32 value_idx = CellMaterial::materials[material_idx].get_value_idx_at(x, y, rng);
			Cell::set_value(cell, value_idx);
			cells[y * width + x] = cell;
		}
	}

	take_border_cells();
}

void Grid::step() {
	ERR_FAIL_COND_MSG(cells == nullptr, "Grid is not initialized");

	Cell::update_updated_bit((u64)Grid::tick);

	Grid::tick++;

	Step::step_borders();

	// Update rows in 3 passes.
	// 0 x x x -
	// 1 o x x 1
	// 2 x o x 2
	// 3 x x o 3
	// 4 o x x 1
	// 5 x o x 2
	// 6 x x o 3
	// 7 o x x 1
	// 8 x x x -
	for (i32 row_start = 1; row_start < 4; row_start++) {
		for (i32 row_idx = row_start; row_idx < chunks_height - 1; row_idx += 3) {
			if (Grid::active_rows[row_idx] == 0) {
				continue;
			}

			tp.push_task(Step::step_row, row_idx);
		}
		tp.wait_for_tasks();
	}
}

void Grid::add_material(
		i32 density,
		i32 movement_vertical_step,
		f32 movement_chance,
		bool horizontal_movement,
		f32 durability,
		i32 collision,
		f32 friction,
		bool can_color,
		// If 0 then no noise. [0..15]
		const u32 max_value_noise,
		// Can be null.
		const Ref<Image> values,
		// [[float probability, int out1, int out2]]
		// Inner array can be empty (no reactions with this material).
		Array reactions,
		u32 cell_biome) {
	std::vector<std::vector<CellReaction>> higher_reactions = {};
	for (i32 i = 0; i < reactions.size(); i++) {
		// Reactions with offset material idx.
		Array r = reactions[i];

		std::vector<CellReaction> inner = {};

		for (int j = 0; j < r.size(); j++) {
			// Reaction data.
			Array rr = r[j];

			CellReaction reaction = CellReaction{
				rr[0],
				rr[1],
				rr[2]
			};

			inner.push_back(reaction);
		}

		higher_reactions.push_back(inner);
	}

	// TODO: Check that enums are valid.
	CellMaterial::add(
			density,
			movement_vertical_step,
			movement_chance,
			horizontal_movement,
			durability,
			static_cast<Cell::Collision>(collision),
			friction,
			can_color,
			max_value_noise,
			values,
			higher_reactions,
			cell_biome);
}

// int min_cell_coverage;
// float min_depth;
// float min_distance_from_center;
void Grid::set_biomes(Array biomes) {
	ERR_FAIL_COND_MSG(biomes.size() == 0, "Biomes array is empty");

	std::vector<Biome> new_biomes = {};

	for (i32 i = 0; i < biomes.size(); i++) {
		Array a = biomes[i];

		ERR_FAIL_COND_MSG(biomes.size() != 3, "Biome array must have 3 elements");

		new_biomes.push_back(Biome{
				a[0],
				a[1],
				a[2] });
	}

	GridBiomeScanner::set_biomes(new_biomes);
}

bool Grid::is_chunk_active(Vector2i chunk_position) {
	if (chunk_position.x < 0 || chunk_position.y < 0 ||
			chunk_position.x >= chunks_width || chunk_position.y >= chunks_height) {
		return false;
	}

	return *(chunks + chunk_position.y * chunks_width + chunk_position.x) != 0;
}

void Grid::set_seed(i64 new_seed) {
	seed = (u64)new_seed;
	rng = seed;
}

i64 Grid::get_seed() {
	return (i64)seed;
}

void Grid::free_memory() {
	CellMaterial::free_memory();
	delete_grid();
}

i64 Grid::get_tick() {
	return tick;
}

u32 Grid::get_cell_material_idx(Vector2i position) {
	return Cell::material_idx(get_cell_checked(position.x, position.y));
}

void Grid::print_materials() {
	print_line("num materials: ", CellMaterial::materials.size());

	for (u32 material_idx = 0; material_idx < CellMaterial::materials.size(); material_idx++) {
		CellMaterial::materials[material_idx].print(material_idx);
	}
}

namespace Test {

void test_activate_rect() {
	u64 *chunk = new u64;
	*chunk = 0;

	Chunk::unsafe_activate_rect(*chunk, 0, 0, 32, 32);
	TEST_ASSERT(*chunk == ~0uLL, "activate full rect: FAIL");
	print_line("activate full rect: OK");

	u64 rng = 12345789;
	for (i32 i = 0; i < 1000; i++) {
		*chunk = 0;

		u32 x_offset = Rng::gen_range_u32(rng, 0, 32);
		u32 y_offset = Rng::gen_range_u32(rng, 0, 32);
		u64 width = Rng::gen_range_u32(rng, 0, 32 - x_offset) + 1;
		u64 height = Rng::gen_range_u32(rng, 0, 32 - y_offset) + 1;

		Chunk::unsafe_activate_rect(*chunk, x_offset, y_offset, width, height);

		TEST_ASSERT(Chunk::active_rect(*chunk).x_start == (i32)x_offset, "x_start");
		TEST_ASSERT(Chunk::active_rect(*chunk).y_start == (i32)y_offset, "y_start");
		TEST_ASSERT(Chunk::active_rect(*chunk).x_end == (i32)(x_offset + width), "x_end");
		TEST_ASSERT(Chunk::active_rect(*chunk).y_end == (i32)(y_offset + height), "y_end");
	}
	print_line("activate random rects: OK");

	delete chunk;
}

void test_rng() {
	u32 num_tests = 100000;
	u32 num_true = 0;

	u64 rng = 12345789;

	for (u32 i = 0; i < num_tests; i++) {
		if (Rng::gen_bool(rng)) {
			num_true++;
		}
	}
	f64 true_bias = (f64)num_true / (f64)num_tests;
	print_line("rng true bias ", true_bias);
	TEST_ASSERT(true_bias > 0.45 && true_bias < 0.55, "rng true bias too high");
	TEST_ASSERT(true_bias != 0.5, "rng true bias is 0.5");

	print_line("rng non-bias: OK");
}

} // namespace Test

void Grid::run_tests() {
	print_line("---------- test_activate_rect: STARTED");
	Test::test_activate_rect();

	print_line("---------- test_rng: STARTED");
	Test::test_rng();

	print_line("---------- All tests passed!");
}

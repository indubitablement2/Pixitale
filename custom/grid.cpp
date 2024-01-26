#include "grid.h"

#include "BS_thread_pool.hpp"
#include "biome.h"
#include "cell.hpp"
#include "cell_material.h"
#include "chunk.h"
#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "preludes.h"
#include "rng.hpp"

inline static BS::thread_pool tp = BS::thread_pool();

void Grid::_bind_methods() {
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("clear"),
			&Grid::clear);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_tick", "value"),
			&Grid::set_tick);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_tick"),
			&Grid::get_tick);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_seed", "value"),
			&Grid::set_seed);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_seed"),
			&Grid::get_seed);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_material_idx", "coord"),
			&Grid::get_cell_material_idx);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_material", "coord"),
			&Grid::get_cell_material);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_chunk_active_rect", "chunk_coord"),
			&Grid::get_chunk_active_rect);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_cell_rect", "rect", "material_idx"),
			&Grid::set_cell_rect);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_buffer", "chunk_rect"),
			&Grid::get_cell_buffer);
}

void Grid::clear() {
	for (auto &[chunk_id, chunk] : chunks) {
		delete chunk;
	}
	chunks = {};

	active_chunks = {};

	tick = 0;
	seed = 0;
}

void Grid::set_tick(u64 value) {
	tick = value;
}

u64 Grid::get_tick() {
	return tick;
}

void Grid::set_seed(u64 value) {
	seed = value;
}

u64 Grid::get_seed() {
	return seed;
}

Rng Grid::get_static_rng(Vector2i chunk_coord) {
	return Rng(chunk_id(chunk_coord) + seed);
}

Rng Grid::get_temporal_rng(Vector2i chunk_coord) {
	return Rng(chunk_id(chunk_coord) + seed + tick);
}

u64 Grid::chunk_id(Vector2i chunk_coord) {
	return (u64)chunk_coord.x | ((u64)chunk_coord.y << 32);
}

Chunk *Grid::get_chunk(Vector2i chunk_coord) {
	if (auto it = chunks.find(chunk_id(chunk_coord)); it != chunks.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

u32 Grid::get_cell_material_idx(Vector2i coord) {
	ChunkLocalCoord chunk_local_coord = ChunkLocalCoord(coord);
	if (auto it = chunks.find(chunk_id(chunk_local_coord.chunk_coord)); it != chunks.end()) {
		return Cell::material_idx(it->second->get_cell(chunk_local_coord.local_coord));
	} else {
		return 0;
	}
}

Ref<CellMaterial> Grid::get_cell_material(Vector2i coord) {
	return CellMaterial::get_material(get_cell_material_idx(coord));
}

Rect2i Grid::get_chunk_active_rect(Vector2i chunk_coord) {
	if (auto it = chunks.find(chunk_id(chunk_coord)); it != chunks.end()) {
		return it->second->active_rect();
	} else {
		return Rect2i();
	}
}

void Grid::set_cell_rect(Rect2i rect, u32 material_idx) {
	if (rect.get_area() <= 0) {
		return;
	}

	// Set cell.
	IterChunk chunk_iter = IterChunk(rect);
	while (chunk_iter.next()) {
		Chunk *chunk = get_chunk(chunk_iter.chunk_coord);

		if (chunk == nullptr) {
			continue;
		}

		Iter2D cell_iter = chunk_iter.local_iter();
		while (cell_iter.next()) {
			// TODO: set color
			u32 cell = Cell::build_cell(material_idx);
			chunk->set_cell(cell_iter.coord, cell);
		}
	}

	// Activate chunks.
	chunk_iter = IterChunk(Rect2i(
			rect.position - Vector2i(1, 1),
			rect.size + Vector2i(2, 2)));
	while (chunk_iter.next()) {
		Chunk *chunk = get_chunk(chunk_iter.chunk_coord);

		if (chunk == nullptr) {
			continue;
		}

		chunk->activate_rect(Rect2i(
				chunk_iter.local_coord_start,
				chunk_iter.local_coord_end - chunk_iter.local_coord_start));
	}
}

Ref<Image> Grid::get_cell_buffer(Rect2i chunk_rect) {
	Vector2i image_size = chunk_rect.size * 32;

	auto image_data = Vector<u8>();
	image_data.resize(image_size.x * image_size.y * 4);
	auto image_buffer = reinterpret_cast<u32 *>(image_data.ptrw());

	Iter2D chunk_iter = Iter2D(Vector2i(), chunk_rect.size);
	while (chunk_iter.next()) {
		Vector2i chunk_coord = chunk_rect.position + chunk_iter.coord;
		Vector2i image_offset = chunk_iter.coord * 32;

		Iter2D cell_iter = Iter2D(Vector2i(), Vector2i(32, 32));

		Chunk *chunk = get_chunk(chunk_coord);
		if (chunk == nullptr) {
			while (cell_iter.next()) {
				Vector2i image_coord = image_offset + cell_iter.coord;
				image_buffer[image_coord.x + image_coord.y * image_size.x] = 0;
			}
		} else {
			while (cell_iter.next()) {
				u32 cell = chunk->get_cell(cell_iter.coord);
				Vector2i image_coord = image_offset + cell_iter.coord;
				image_buffer[image_coord.x + image_coord.y * image_size.x] = cell;
			}
		}
	}

	return Image::create_from_data(
			image_size.x,
			image_size.y,
			false,
			Image::FORMAT_RF,
			image_data);
}

void Grid::step() {
	// TODO

	// 	ERR_FAIL_COND_MSG(cells == nullptr, "Grid is not initialized");

	// 	Cell::update_updated_bit((u64)Grid::tick);

	// 	Grid::tick++;

	// 	Step::apply_active_borders();
	// 	Step::apply_static_borders();

	// 	// Update rows in 3 passes.
	// 	for (i32 row_start = 1; row_start < 4; row_start++) {
	// 		for (i32 row_idx = row_start; row_idx < chunks_height - 1; row_idx += 3) {
	// 			if (Grid::active_rows[row_idx] == 0) {
	// 				continue;
	// 			}

	// 			tp.push_task(Step::step_row, row_idx);
	// 		}
	// 		tp.wait_for_tasks();
	// 	}
}

#include "grid.h"
#include "BS_thread_pool.hpp"
#include "biome.h"
#include "cell.hpp"
#include "cell_material.h"
#include "chunk.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/os/memory.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "preludes.h"
#include "rng.hpp"
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

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
			D_METHOD("get_cell_buffer", "chunk_rect"),
			&Grid::get_cell_buffer);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("iter_chunk", "chunk_coord", "activate"),
			&Grid::iter_chunk);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("iter", "rect"),
			&Grid::iter_rect);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("queue_step_chunks", "chunk_rect"),
			&Grid::queue_step_chunks);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("step_prepare"),
			&Grid::step_prepare);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("step_start"),
			&Grid::step_start);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("step_wait_to_finish"),
			&Grid::step_wait_to_finish);
}

void Grid::clear_iters() {
	for (auto &iter : chunk_iters) {
		if (iter->next()) {
			continue;
		} else {
			memdelete(iter);
		}
	}
	chunk_iters.clear();

	for (auto &iter : rect_iters) {
		if (iter->next()) {
			continue;
		} else {
			memdelete(iter);
		}
	}
	rect_iters.clear();
}

void Grid::clear() {
	step_wait_to_finish();

	clear_iters();
	queue_step_chunk_rects = {};

	for (auto &[chunk_id, chunk] : chunks) {
		delete chunk;
	}
	chunks = {};

	active_chunks = {};

	tick = 0;
	seed = 0;
}

void Grid::set_tick(i64 value) {
	tick = value;
	temporal_rng = Rng(seed + tick);
}

i64 Grid::get_tick() {
	return tick;
}

void Grid::set_seed(u64 value) {
	seed = value;
	temporal_rng = Rng(seed + tick);
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

CellMaterial *Grid::get_cell_material(Vector2i coord) {
	return CellMaterial::get_material(get_cell_material_idx(coord));
}

Rect2i Grid::get_chunk_active_rect(Vector2i chunk_coord) {
	if (auto it = chunks.find(chunk_id(chunk_coord)); it != chunks.end()) {
		return it->second->active_rect();
	} else {
		return Rect2i();
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

GridRectIter *Grid::iter_rect(Rect2i rect) {
	if (rect.size.x <= 0 || rect.size.y <= 0) {
		return nullptr;
	}

	GridRectIter *iter = new GridRectIter(rect);
	postinitialize_handler(iter);

	rect_iters.push_back(iter);

	return iter;
}

GridChunkIter *Grid::iter_chunk(Vector2i chunk_coord, bool activate) {
	GridChunkIter *iter = new GridChunkIter(chunk_coord, activate);
	postinitialize_handler(iter);

	chunk_iters.push_back(iter);

	if (activate) {
	}

	return iter;
}

void Grid::queue_step_chunks(Rect2i chunk_rect) {
	if (chunk_rect.size.x > 0 && chunk_rect.size.y > 0) {
		queue_step_chunk_rects.push_back(chunk_rect);
	}
}

void Grid::step_prepare() {
	set_tick(tick + 1);

	clear_iters();

	for (i32 i = 0; i < 3; i++) {
		passes[i].clear();
	}

	for (auto chunk_rect : queue_step_chunk_rects) {
		// Add to-be-generated chunks.
		Iter2D chunk_iter = Iter2D(
				chunk_rect.position - Vector2i(1, 1),
				chunk_rect.get_end() + Vector2i(2, 2));
		while (chunk_iter.next()) {
			auto added = chunks.emplace(chunk_id(chunk_iter.coord), nullptr);
			if (added.second) {
				added.first->second = new Chunk();
			}
		}

		chunk_iter = Iter2D(chunk_rect);
		while (chunk_iter.next()) {
			i32 pass_idx = mod_neg(chunk_iter.coord.x, 3);
			passes[pass_idx][chunk_iter.coord.x].push_back(chunk_iter.coord.y);
		}
	}
	queue_step_chunk_rects.clear();
}

void Grid::step_start() {
	for (i32 i = 0; i < 3; i++) {
		auto &pass = passes[i];

		for (auto &pair : pass) {
			tp.push_task([&pair] {
				std::sort(pair.second.begin(), pair.second.end());

				i32 x = pair.first;
				i32 y = MAX_I32;

				// Iterate from bottom to top.
				for (auto it = pair.second.rbegin(); it != pair.second.rend(); it++) {
					if (y == *it) {
						continue;
					}
					y = *it;

					Chunk::step_chunk(Vector2i(x, y));
				}
			});
		}
	}
}

void Grid::step_wait_to_finish() {
	tp.wait_for_tasks();
}

bool Grid::randb() {
	return temporal_rng.gen_bool();
}

bool Grid::randb_probability(f32 probability) {
	return temporal_rng.gen_probability_f32(probability);
}

f32 Grid::randf() {
	return temporal_rng.gen_f32();
}

f32 Grid::randf_range(f32 min, f32 max) {
	return temporal_rng.gen_range_f32(min, max);
}

i32 Grid::randi_range(i32 min, i32 max) {
	return temporal_rng.gen_range_i32(min, max);
}
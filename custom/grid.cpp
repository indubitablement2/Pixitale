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
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "generation_pass.h"
#include "grid_iter.h"
#include "preludes.h"
#include "rng.hpp"
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

inline static BS::thread_pool tp = BS::thread_pool();

u32 reations_key(const u32 m1, const u32 m2, bool &swap) {
	if (m1 <= m2) {
		swap = false;
		return m1 | (m2 << 16);
	} else {
		swap = true;
		return m2 | (m1 << 16);
	}
}

void Grid::_bind_methods() {
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("clear_cell_materials"),
			&Grid::clear_cell_materials);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("add_cell_material", "obj"),
			&Grid::add_cell_material);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("clear_cell_reactions"),
			&Grid::clear_cell_reactions);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("add_cell_reaction", "in1", "in2", "out1", "out2", "probability"),
			&Grid::add_cell_reaction);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("remove_cell_reaction", "reaction_id"),
			&Grid::remove_cell_reaction);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("clear_generation_passes"),
			&Grid::clear_generation_passes);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("add_generation_pass", "value"),
			&Grid::add_generation_pass);

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
			D_METHOD("get_chunk_active_rect", "chunk_coord"),
			&Grid::get_chunk_active_rect);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_buffer", "chunk_rect"),
			&Grid::get_cell_buffer);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("iter_chunk", "chunk_coord"),
			&Grid::iter_chunk);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("iter_rect", "rect"),
			&Grid::iter_rect);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("force_step"),
			&Grid::force_step);
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

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("randb"),
			&Grid::randb);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("randb_probability", "probability"),
			&Grid::randb_probability);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("randf"),
			&Grid::randf);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("randf_range", "min", "max"),
			&Grid::randf_range);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("randi_range", "min", "max"),
			&Grid::randi_range);

	BIND_ENUM_CONSTANT(CELL_COLLISION_NONE);
	BIND_ENUM_CONSTANT(CELL_COLLISION_SOLID);
	BIND_ENUM_CONSTANT(CELL_COLLISION_PLATFORM);
	BIND_ENUM_CONSTANT(CELL_COLLISION_LIQUID);
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

void Grid::reactions_between(
		CellReaction *&start,
		CellReaction *&end,
		u32 m1,
		u32 m2,
		bool &swap) {
	u32 reactions_key = reations_key(
			m1,
			m2,
			swap);

	if (auto it = cell_reactions.find(reactions_key); it != cell_reactions.end()) {
		auto v = &it->second;
		start = v->data();
		end = start + v->size();
	} else {
		start = nullptr;
		end = nullptr;
	}
}

void Grid::generate_chunk(Vector2i chunk_coord) {
	GridChunkIter *iter = new GridChunkIter(chunk_coord, false);
	postinitialize_handler(iter);

	for (auto pass : generation_passes) {
		iter->reset_iter();
		pass->generate(iter);
	}

	memdelete(iter);
}

CellMaterial &Grid::get_cell_material(u32 material_idx) {
	if (material_idx < cell_materials.size()) {
		return cell_materials[material_idx];
	} else {
		return cell_materials[0];
	}
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

void Grid::clear_cell_materials() {
	cell_materials.clear();
}

void Grid::add_cell_material(Object *obj) {
	cell_materials.push_back(CellMaterial(obj));
}

void Grid::clear_cell_reactions() {
	cell_reactions.clear();
}

u64 Grid::add_cell_reaction(u32 in1, u32 in2, u32 out1, u32 out2, f64 probability) {
	// TODO: check if mat idx are valid

	last_modified_tick = tick;

	bool swap = false;
	u32 reaction_key = reations_key(in1, in2, swap);

	CellReaction reaction = {};

	reaction.probability = u16(CLAMP(
			probability * (f64)CELL_REACTION_PROBABILITY_RANGE,
			0.0,
			(f64)(MAX_U16)));

	if (swap) {
		reaction.mat_idx_out1 = out2;
		reaction.mat_idx_out2 = out1;
	} else {
		reaction.mat_idx_out1 = out1;
		reaction.mat_idx_out2 = out2;
	}

	reaction.callback = nullptr;

	reaction.reaction_id = 0;

	if (auto it = cell_reactions.find(reaction_key); it != cell_reactions.end()) {
		for (auto &r : it->second) {
			reaction.reaction_id = MAX(reaction.reaction_id, r.reaction_id);
		}
		reaction.reaction_id += 1;
		it->second.push_back(reaction);
	} else {
		cell_reactions[reaction_key] = { reaction };
	}

	return reaction_key | (u64(reaction.reaction_id) << 32);
}

bool Grid::remove_cell_reaction(u64 reaction_id) {
	last_modified_tick = tick;

	u32 key = u32(reaction_id & 0xFFFFFFFF);
	u16 id = u16(reaction_id >> 32);

	if (auto it = cell_reactions.find(key); it != cell_reactions.end()) {
		auto &reactions = it->second;
		for (auto itt = reactions.begin(); itt != reactions.end(); itt++) {
			if (itt->reaction_id == id) {
				reactions.erase(itt);
				return true;
			}
		}
	}

	return false;
}

void Grid::clear_generation_passes() {
	generation_passes = {};
}

void Grid::add_generation_pass(GenerationPass *value) {
	generation_passes.push_back(value);
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

u32 Grid::get_cell_material_idx(Vector2i coord) {
	ChunkLocalCoord chunk_local_coord = ChunkLocalCoord(coord);
	Chunk *chunk = get_chunk(chunk_local_coord.chunk_coord);
	if (chunk != nullptr) {
		return Cell::material_idx(chunk->get_cell(chunk_local_coord.local_coord));
	} else {
		return 0;
	}
}

Rect2i Grid::get_chunk_active_rect(Vector2i chunk_coord) {
	Chunk *chunk = get_chunk(chunk_coord);
	if (chunk != nullptr) {
		return chunk->active_rect();
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

GridChunkIter *Grid::iter_chunk(Vector2i chunk_coord) {
	GridChunkIter *iter = new GridChunkIter(chunk_coord, true);
	postinitialize_handler(iter);

	chunk_iters.push_back(iter);

	return iter;
}

void Grid::force_step() {
	last_modified_tick = tick;
}

void Grid::queue_step_chunks(Rect2i chunk_rect) {
	if (chunk_rect.size.x > 0 && chunk_rect.size.y > 0) {
		queue_step_chunk_rects.push_back(chunk_rect);
	}
}

void Grid::step_prepare() {
	set_tick(tick + 1);

	cell_updated_bitmask = ((tick % 3) + 1) << Cell::Shifts::SHIFT_UPDATED;

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
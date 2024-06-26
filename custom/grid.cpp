#include "grid.h"
#include "biome.h"
#include "cell.hpp"
#include "cell_material.hpp"
#include "chunk.h"
#include "core/core_bind.h"
#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/string/print_string.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/typed_array.h"
#include "grid_iter.h"
#include "preludes.h"
#include "rng.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

inline static std::unordered_map<Thread::ID, std::vector<std::pair<Callable *, Vector2i>>> thread_vectors = {};
inline static Mutex thread_vectors_mutex = Mutex();

u32 reations_key(const u32 m1, const u32 m2, bool &swap) {
	if (m1 <= m2) {
		swap = false;
		return m1 | (m2 << 16);
	} else {
		swap = true;
		return m2 | (m1 << 16);
	}
}

i32 get_slice_idx(i32 x) {
	return div_floor(x + (GENERATION_SLICE_CHUNK_SIZE / 2), GENERATION_SLICE_CHUNK_SIZE);
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
			D_METHOD("add_cell_reaction", "in1", "in2", "out1", "out2", "probability", "callback"),
			&Grid::add_cell_reaction);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("remove_cell_reaction", "reaction_id"),
			&Grid::remove_cell_reaction);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("print_internals"),
			&Grid::print_internals);

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
			D_METHOD("get_chunk_active_rect", "chunk_coord"),
			&Grid::get_chunk_active_rect);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_grid_memory_usage"),
			&Grid::get_grid_memory_usage);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_buffer", "rect", "background", "clean"),
			&Grid::get_cell_buffer);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_material_idx", "coord"),
			&Grid::get_cell_material_idx_v);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_cell_color", "coord"),
			&Grid::get_cell_color_v);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_cell_material_idx", "coord", "material_idx"),
			&Grid::set_cell_material_idx_v);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("set_cell_color", "coord", "color"),
			&Grid::set_cell_color_v);

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
			D_METHOD("iter_line", "start", "end"),
			&Grid::iter_line);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("iter_fill", "start", "filter_material_idx"),
			&Grid::iter_fill);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("get_line", "start", "end"),
			&Grid::get_line);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("chunk_exists"),
			&Grid::chunk_exists);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("try_create_chunk", "chunk_coord"),
			&Grid::try_create_chunk);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("step_chunk", "chunk_coord"),
			&Grid::step_chunk);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("pre_step"),
			&Grid::pre_step);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("post_step"),
			&Grid::post_step);

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

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("color_to_material_idx", "color"),
			&Grid::color_to_material_idx);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("color_to_color_idx", "color"),
			&Grid::color_to_color_idx);

	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("div_floor", "numerator", "denominator"),
			&Grid::_div_floor);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("div_floor_v", "numerator", "denominator"),
			&Grid::_div_floor_v);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("mod_neg", "numerator", "denominator"),
			&Grid::_mod_neg);
	ClassDB::bind_static_method(
			"Grid",
			D_METHOD("mod_neg_v", "numerator", "denominator"),
			&Grid::_mod_neg_v);

	BIND_CONSTANT(GENERATION_SLICE_CHUNK_SIZE);

	BIND_ENUM_CONSTANT(CELL_COLLISION_NONE);
	BIND_ENUM_CONSTANT(CELL_COLLISION_SOLID);
	BIND_ENUM_CONSTANT(CELL_COLLISION_PLATFORM);
	BIND_ENUM_CONSTANT(CELL_COLLISION_LIQUID);
}

std::vector<std::pair<Callable *, Vector2i>> &Grid::get_reaction_callback_vector() {
	Thread::ID id = Thread::get_caller_id();

	thread_vectors_mutex.lock();
	auto &vec = thread_vectors[id];
	thread_vectors_mutex.unlock();

	return vec;
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

const CellMaterial &Grid::get_cell_material(u32 material_idx) {
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
	// Casting signed to larger unsigned use sign extends (movsx),
	// so we cast to u32 then to u64 (mov).
	return u64(u32(chunk_coord.x)) | (u64(u32(chunk_coord.y)) << 32);
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

u64 Grid::add_cell_reaction(u32 in1, u32 in2, u32 out1, u32 out2, f64 probability, Callable callback) {
	ERR_FAIL_COND_V_MSG(
			in1 >= cell_materials.size(),
			0,
			"unknow cell material idx for in1");
	ERR_FAIL_COND_V_MSG(
			in2 >= cell_materials.size(),
			0,
			"unknow cell material idx for in2");
	ERR_FAIL_COND_V_MSG(
			out1 >= cell_materials.size(),
			0,
			"unknow cell material idx for out1");
	ERR_FAIL_COND_V_MSG(
			out2 >= cell_materials.size(),
			0,
			"unknow cell material idx for out2");

	last_modified_tick = tick;

	bool swap = false;
	u32 reaction_key = reations_key(in1, in2, swap);

	CellReaction reaction = {};

	reaction.probability = u32(CLAMP(
			probability * f64(MAX_U32),
			0.0,
			f64(MAX_U32)));

	reaction.callback_swap = swap;
	if (swap) {
		reaction.mat_idx_out1 = out2;
		reaction.mat_idx_out2 = out1;
	} else {
		reaction.mat_idx_out1 = out1;
		reaction.mat_idx_out2 = out2;
	}

	reaction.callback = callback;

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

	return u64(reaction_key) | (u64(reaction.reaction_id) << 32);
}

bool Grid::remove_cell_reaction(u64 reaction_id) {
	last_modified_tick = tick;

	u32 key = u32(reaction_id & 0xFFFFFFFF);
	u32 id = u32(reaction_id >> 32);

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

void Grid::print_internals() {
	i32 cell_material_idx = 0;
	for (auto &cell_material : cell_materials) {
		print_line("material:", cell_material_idx);
		print_line("	collision:", cell_material.collision);
		print_line("	density:", cell_material.density);
		print_line("	vertical_movement:", cell_material.vertical_movement);
		print_line("	horizontal_movement:", cell_material.horizontal_movement);
		print_line("	horizontal_movement_start_chance:", f64(cell_material.horizontal_movement_start_chance) / f64(MAX_U32));
		print_line("	horizontal_movement_stop_chance:", f64(cell_material.horizontal_movement_stop_chance) / f64(MAX_U32));
		print_line("	noise_darken_max:", cell_material.noise_darken_max);
		print_line("	dissipate_on_horizontal_movement:", cell_material.dissipate_on_horizontal_movement);
		print_line("	can_reverse_horizontal_movement:", cell_material.can_reverse_horizontal_movement);
		print_line("	can_color:", cell_material.can_color);

		cell_material_idx += 1;
	}

	for (auto &[key, reactions] : cell_reactions) {
		u32 in1 = key & 0xFFFF;
		u32 in2 = key >> 16;
		print_line("reactions between:", in1, "and", in2);
		for (auto &reaction : reactions) {
			print_line("	id:", u64(key) | (u64(reaction.reaction_id) << 32));
			print_line("	probability:", f64(reaction.probability) / f64(MAX_U32));
			print_line("	out1:", reaction.mat_idx_out1);
			print_line("	out2:", reaction.mat_idx_out2);
			print_line("	callback:", !reaction.callback.is_null());
			print_line("	callback valid:", reaction.callback.is_valid());
		}
	}
}

void Grid::clear() {
	for (auto &[chunk_id, chunk] : chunks) {
		delete chunk;
	}
	chunks = {};

	tick = 0;
	seed = 0;
}

void Grid::set_tick(i64 value) {
	tick = value;
	temporal_rng = Rng(tick + seed);
	cell_updated_bitmask = (u32(tick % 3) + 1u) << Cell::Shifts::SHIFT_UPDATED;
}

i64 Grid::get_tick() {
	return tick;
}

void Grid::set_seed(u64 value) {
	seed = value;
}

u64 Grid::get_seed() {
	return seed;
}

Rect2i Grid::get_chunk_active_rect(Vector2i chunk_coord) {
	Chunk *chunk = get_chunk(chunk_coord);
	if (chunk == nullptr) {
		return Rect2i();
	} else {
		return chunk->active_rect();
	}
}

i64 Grid::get_grid_memory_usage() {
	i64 mem = 0;
	for (auto &pair : chunks) {
		mem += pair.second->get_memory_usage();
	}
	return mem;
}

Ref<Image> Grid::get_cell_buffer(Rect2i rect, bool background, bool clean) {
	// Tried not creating a new buffer each time, but it was not noticeably faster.
	auto image_data = Vector<u8>();
	image_data.resize(rect.size.x * 4 * rect.size.y);
	u32 *image_buffer = reinterpret_cast<u32 *>(image_data.ptrw());

	// This is where 99% of the time is spent.
	IterChunk chunk_iter = IterChunk(rect);
	while (chunk_iter.next()) {
		Chunk *chunk = get_chunk(chunk_iter.chunk_coord);
		u32 *cells;
		if (chunk == nullptr) {
			cells = nullptr;
		} else if (background) {
			cells = chunk->background;
		} else {
			cells = &chunk->cells[0];
		}

		Vector2i image_offset = chunk_iter.chunk_coord * 32 - rect.position;
		Iter2D local_iter = chunk_iter.local_iter();
		if (cells == nullptr) {
			while (local_iter.next()) {
				u32 *img_ptr = image_buffer + image_offset.x + local_iter.coord.x + (image_offset.y + local_iter.coord.y) * rect.size.x;
				*img_ptr = 0;
			}
		} else {
			while (local_iter.next()) {
				u32 *img_ptr = image_buffer + image_offset.x + local_iter.coord.x + (image_offset.y + local_iter.coord.y) * rect.size.x;
				u32 cell = cells[local_iter.coord.x + local_iter.coord.y * 32];
				if (clean) {
					if (background) {
						Cell::clean_background(cell);
					} else {
						Cell::clean(cell);
					}
				}
				*img_ptr = cell;
			}
		}
	}

	return Image::create_from_data(
			rect.size.x,
			rect.size.y,
			false,
			Image::FORMAT_RF,
			image_data);
}

PackedByteArray Grid::get_chunk_state(Vector2i chunk_coord) {
	return PackedByteArray(); // todo
}

u32 Grid::get_cell_data(ChunkLocalCoord coord) {
	Chunk *chunk = get_chunk(coord.chunk_coord);
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get_cell(coord.local_coord);
}

u32 Grid::get_cell_material_idx(ChunkLocalCoord coord) {
	return Cell::material_idx(get_cell_data(coord));
}

u32 Grid::get_cell_color(ChunkLocalCoord coord) {
	return Cell::color(get_cell_data(coord));
}

void Grid::set_cell_material_idx(ChunkLocalCoord coord, u32 material_idx) {
	ERR_FAIL_COND_MSG(material_idx >= cell_materials.size(), "material_idx must be less than cell_materials.size");

	Chunk *chunk = get_chunk(coord.chunk_coord);
	if (chunk == nullptr) {
		return;
	}

	CellMaterial &mat = cell_materials[material_idx];
	if (mat.noise_darken_max > 0) {
		Cell::set_darken(material_idx, temporal_rng.gen_range_u32(0, mat.noise_darken_max));
	}

	chunk->set_cell(coord.local_coord, material_idx);

	// Activate neighboring cells.
	IterChunk chunk_iter = IterChunk(coord - Vector2i(1, 1), coord + Vector2i(2, 2));
	while (chunk_iter.next()) {
		chunk = get_chunk(chunk_iter.chunk_coord);
		if (chunk == nullptr) {
			continue;
		}

		chunk->activate_rect(chunk_iter.local_rect());

		Iter2D cell_iter = chunk_iter.local_iter();
		while (cell_iter.next()) {
			Cell::set_active(*chunk->get_cell_ptr(cell_iter.coord), true);
		}
	}
}

void Grid::set_cell_color(ChunkLocalCoord coord, u32 color) {
	Chunk *chunk = get_chunk(coord.chunk_coord);
	if (chunk == nullptr) {
		return;
	}

	u32 *cell = chunk->get_cell_ptr(coord.local_coord);

	if (get_cell_material(Cell::material_idx(*cell)).can_color) {
		Cell::set_color(*cell, color);
	}
}

u32 Grid::get_cell_data_v(Vector2i coord) {
	return get_cell_data(ChunkLocalCoord(coord));
}

u32 Grid::get_cell_material_idx_v(Vector2i coord) {
	return get_cell_material_idx(ChunkLocalCoord(coord));
}

u32 Grid::get_cell_color_v(Vector2i coord) {
	return get_cell_color(ChunkLocalCoord(coord));
}

void Grid::set_cell_material_idx_v(Vector2i coord, u32 material_idx) {
	set_cell_material_idx(ChunkLocalCoord(coord), material_idx);
}

void Grid::set_cell_color_v(Vector2i coord, u32 color) {
	set_cell_color(ChunkLocalCoord(coord), color);
}

Ref<GridChunkIter> Grid::iter_chunk(Vector2i chunk_coord) {
	ERR_FAIL_NULL_V_MSG(
			get_chunk(chunk_coord),
			Ref<GridChunkIter>(),
			"GridChunkIter needs chunk to exist (unlike other iterators)");

	Ref<GridChunkIter> iter = memnew(GridChunkIter);
	iter->prepare(chunk_coord);
	return iter;
}

Ref<GridRectIter> Grid::iter_rect(Rect2i rect) {
	if (rect.size.x <= 0 || rect.size.y <= 0) {
		rect.size = Vector2i(0, 0);
	}

	Ref<GridRectIter> iter = memnew(GridRectIter);
	iter->prepare(rect);
	return iter;
}

Ref<GridLineIter> Grid::iter_line(Vector2i start, Vector2i end) {
	Ref<GridLineIter> iter = memnew(GridLineIter);
	iter->prepare(start, end);
	return iter;
}

Ref<GridFillIter> Grid::iter_fill(Vector2i start, u32 filter_material_idx) {
	Ref<GridFillIter> iter = memnew(GridFillIter);
	iter->prepare(filter_material_idx, start);
	return iter;
}

TypedArray<Vector2i> Grid::get_line(Vector2i start, Vector2i end) {
	TypedArray<Vector2i> line;
	IterLine line_iter = IterLine(end - start);
	line.resize(i32(line_iter.num_points));
	while (line_iter.next()) {
		line.set(i32(line_iter.p), line_iter.currenti() + start);
	}
	return line;
}

bool Grid::chunk_exists(Vector2i chunk_coord) {
	return get_chunk(chunk_coord) != nullptr;
}

bool Grid::try_create_chunk(Vector2i chunk_coord) {
	auto added = chunks.emplace(chunk_id(chunk_coord), nullptr);
	if (added.second) {
		added.first->second = new Chunk();
		added.first->second->chunk_coord = chunk_coord;
		return true;
	}
	return false;
}

void Grid::step_chunk(Vector2i chunk_coord) {
	Chunk::step_chunk(chunk_coord);
}

void Grid::pre_step() {}

void Grid::post_step() {
	// Reactions callback.
	std::vector<std::pair<Callable *, Vector2i>> vec = {};
	u64 reserve_size = 0;
	for (auto &[id, v] : thread_vectors) {
		reserve_size += v.size();
	}
	// print_line(reserve_size);
	vec.reserve(reserve_size);
	for (auto &[id, v] : thread_vectors) {
		vec.insert(vec.end(), v.begin(), v.end());
	}
	thread_vectors.clear();
	std::sort(vec.begin(), vec.end(), [](auto &a, auto &b) {
		return a.second < b.second;
	});
	for (auto &[callable, coord] : vec) {
		callable->call(coord);
	}

	if (tick % 1024 == 0) {
		// i64 unload_threshold = tick - 120 - MAX(3600 - i64(chunks.size()), 0);

		for (auto it = chunks.begin(); it != chunks.end();) {
			Chunk &chunk = *it->second;

			// Delete empty background.
			if (chunk.num_background_cell == 0) {
				delete[] chunk.background;
				chunk.background = nullptr;
			}

			it++;

			// if (chunk.last_step_tick < unload_threshold) {
			// 	unload_chunk_callback.call(chunk.chunk_coord);
			// 	delete it->second;
			// 	it = chunks.erase(it);
			// } else {
			// 	++it;
			// }
		}
	}
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

u32 Grid::color_to_material_idx(Color color) {
	f32 f = color.r;
	u32 u;
	std::memcpy(&u, &f, sizeof(u));
	return Cell::material_idx(u);
}

u32 Grid::color_to_color_idx(Color color) {
	f32 f = color.r;
	u32 u;
	std::memcpy(&u, &f, sizeof(u));
	return Cell::color(u);
}

i64 Grid::_div_floor(i64 numerator, i64 denominator) {
	TEST_ASSERT(denominator > 0, "denominator is not greater than 0");

	if (numerator >= 0) {
		return numerator / denominator;
	} else {
		return -1 - (-1 - numerator) / denominator;
	}
}

Vector2i Grid::_div_floor_v(Vector2i numerator, Vector2i denominator) {
	return div_floor(numerator, denominator);
}

i64 Grid::_mod_neg(i64 numerator, i64 denominator) {
	TEST_ASSERT(denominator > 0, "denominator is not greater than 0");

	i64 mod = numerator % denominator;
	if (mod >= 0) {
		return mod;
	} else {
		return mod + denominator;
	}
}

Vector2i Grid::_mod_neg_v(Vector2i numerator, Vector2i denominator) {
	return mod_neg(numerator, denominator);
}

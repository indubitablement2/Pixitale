#include "grid_iter.h"
#include "cell.hpp"
#include "cell_material.hpp"
#include "core/error/error_macros.h"
#include "core/math/vector2i.h"
#include "grid.h"
#include "preludes.h"
#include <queue>

// For breadth first search.
static i32 seen[512 * 512] = { 0 };
static i32 current_seen_id = 0;
// In seen coord.
static std::queue<Vector2i> next_queue = {};

void GridChunkIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridChunkIter::next);

	ClassDB::bind_method(D_METHOD("get_material_idx"), &GridChunkIter::get_material_idx);
	ClassDB::bind_method(D_METHOD("get_color"), &GridChunkIter::get_color);
	ClassDB::bind_method(D_METHOD("set_material_idx", "material_idx"), &GridChunkIter::set_material_idx);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &GridChunkIter::set_color);

	ClassDB::bind_method(D_METHOD("fill_remaining", "material_idx"), &GridChunkIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridChunkIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("chunk_local_coord"), &GridChunkIter::chunk_local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridChunkIter::coord);

	ClassDB::bind_method(D_METHOD("reset_iter"), &GridChunkIter::reset_iter);

	ClassDB::bind_method(D_METHOD("randb"), &GridChunkIter::randb);
	ClassDB::bind_method(D_METHOD("randb_probability", "probability"), &GridChunkIter::randb_probability);
	ClassDB::bind_method(D_METHOD("randf"), &GridChunkIter::randf);
	ClassDB::bind_method(D_METHOD("randf_range", "min", "max"), &GridChunkIter::randf_range);
	ClassDB::bind_method(D_METHOD("randi_range", "min", "max"), &GridChunkIter::randi_range);
}

void GridChunkIter::prepare(Vector2i p_chunk_coord) {
	x = -1;
	y = 0;
	_chunk_coord = p_chunk_coord;
	chunk = Grid::get_chunk(p_chunk_coord);
	rng = Grid::get_static_rng(p_chunk_coord);
}

bool GridChunkIter::next() {
	x += 1;
	if (x >= 32) {
		x = 0;
		y += 1;
		if (y >= 32) {
			y = 0;
			x = -1;
			return false;
		}
	}
	return true;
}

u32 GridChunkIter::get_material_idx() {
	return Cell::material_idx(chunk->get_cell(chunk_local_coord()));
}

u32 GridChunkIter::get_color() {
	return Cell::color(chunk->get_cell(chunk_local_coord()));
}

void GridChunkIter::set_material_idx(u32 material_idx) {
	ERR_FAIL_COND_MSG(material_idx >= Grid::cell_materials.size(), "material_idx must be less than cell_materials.size");

	CellMaterial &mat = Grid::cell_materials[material_idx];
	if (mat.noise_darken_max > 0) {
		Cell::set_darken(material_idx, rng.gen_range_u32(0, mat.noise_darken_max));
	}
	chunk->set_cell(chunk_local_coord(), material_idx);
}

void GridChunkIter::set_color(u32 color) {
	u32 *cell_ptr = chunk->get_cell_ptr(chunk_local_coord());

	if (Grid::get_cell_material(Cell::material_idx(*cell_ptr)).can_color) {
		Cell::set_color(*cell_ptr, color);
	}
}

void GridChunkIter::fill_remaining(u32 material_idx) {
	while (next()) {
		set_material_idx(material_idx);
	}
}

Vector2i GridChunkIter::chunk_coord() {
	return _chunk_coord;
}

Vector2i GridChunkIter::chunk_local_coord() {
	return Vector2i(x & 31, y);
}

Vector2i GridChunkIter::coord() {
	return chunk_coord() * 32 + chunk_local_coord();
}

void GridChunkIter::reset_iter() {
	x = -1;
	y = 0;
}

bool GridChunkIter::randb() {
	return rng.gen_bool();
}

bool GridChunkIter::randb_probability(f32 probability) {
	return rng.gen_probability_f32(probability);
}

f32 GridChunkIter::randf() {
	return rng.gen_f32();
}

f32 GridChunkIter::randf_range(f32 min, f32 max) {
	return rng.gen_range_f32(min, max);
}

i32 GridChunkIter::randi_range(i32 min, i32 max) {
	return rng.gen_range_i32(min, max);
}

void GridRectIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridRectIter::next);

	ClassDB::bind_method(D_METHOD("get_material_idx"), &GridRectIter::get_material_idx);
	ClassDB::bind_method(D_METHOD("get_color"), &GridRectIter::get_color);
	ClassDB::bind_method(D_METHOD("set_material_idx", "material_idx"), &GridRectIter::set_material_idx);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &GridRectIter::set_color);

	ClassDB::bind_method(D_METHOD("fill_remaining", "material_idx"), &GridRectIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridRectIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("chunk_local_coord"), &GridRectIter::chunk_local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridRectIter::coord);
}

void GridRectIter::prepare(Rect2i rect) {
	iter = Iter2D(rect);
	current = ChunkLocalCoord(rect.position);
}

bool GridRectIter::next() {
	if (iter.next()) {
		current = ChunkLocalCoord(iter.coord);
		return true;
	} else {
		return false;
	}
}

u32 GridRectIter::get_material_idx() {
	return Grid::get_cell_material_idx(current);
}

u32 GridRectIter::get_color() {
	return Grid::get_cell_color(current);
}

void GridRectIter::set_material_idx(u32 material_idx) {
	Grid::set_cell_material_idx(current, material_idx);
}

void GridRectIter::set_color(u32 color) {
	Grid::set_cell_color(current, color);
}

void GridRectIter::fill_remaining(u32 material_idx) {
	while (next()) {
		set_material_idx(material_idx);
	}
}

Vector2i GridRectIter::chunk_coord() {
	return current.chunk_coord;
}

Vector2i GridRectIter::chunk_local_coord() {
	return current.local_coord;
}

Vector2i GridRectIter::coord() {
	return iter.coord;
}

void GridLineIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridLineIter::next);

	ClassDB::bind_method(D_METHOD("get_material_idx"), &GridLineIter::get_material_idx);
	ClassDB::bind_method(D_METHOD("get_color"), &GridLineIter::get_color);
	ClassDB::bind_method(D_METHOD("set_material_idx", "material_idx"), &GridLineIter::set_material_idx);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &GridLineIter::set_color);

	ClassDB::bind_method(D_METHOD("fill_remaining", "material_idx"), &GridLineIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridLineIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("chunk_local_coord"), &GridLineIter::chunk_local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridLineIter::coord);
}

void GridLineIter::prepare(Vector2i p_start, Vector2i p_end) {
	start = p_start;
	iter = IterLine(p_end - p_start);
}

bool GridLineIter::next() {
	if (iter.next()) {
		current = ChunkLocalCoord(iter.currenti() + start);
		return true;
	} else {
		return false;
	}
}

u32 GridLineIter::get_material_idx() {
	return Grid::get_cell_material_idx(current);
}

u32 GridLineIter::get_color() {
	return Grid::get_cell_color(current);
}

void GridLineIter::set_material_idx(u32 material_idx) {
	Grid::set_cell_material_idx(current, material_idx);
}

void GridLineIter::set_color(u32 color) {
	Grid::set_cell_color(current, color);
}

void GridLineIter::fill_remaining(u32 material_idx) {
	while (next()) {
		set_material_idx(material_idx);
	}
}

Vector2i GridLineIter::chunk_coord() {
	return current.chunk_coord;
}

Vector2i GridLineIter::chunk_local_coord() {
	return current.local_coord;
}

Vector2i GridLineIter::coord() {
	return current.coord();
}

void GridFillIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridFillIter::next);

	ClassDB::bind_method(D_METHOD("get_material_idx"), &GridFillIter::get_material_idx);
	ClassDB::bind_method(D_METHOD("get_color"), &GridFillIter::get_color);
	ClassDB::bind_method(D_METHOD("set_material_idx", "material_idx"), &GridFillIter::set_material_idx);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &GridFillIter::set_color);

	ClassDB::bind_method(D_METHOD("fill_remaining", "material_idx"), &GridFillIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridFillIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("chunk_local_coord"), &GridFillIter::chunk_local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridFillIter::coord);
}

void GridFillIter::prepare(u32 p_filter_material_idx, Vector2i p_start) {
	filter_material_idx = p_filter_material_idx;
	start = p_start;

	while (!next_queue.empty()) {
		next_queue.pop();
	}

	current_seen_id += 1;
	seen_id = current_seen_id;

	next_queue.push(Vector2i(256, 256));
	seen[256 * 512 + 256] = current_seen_id;
}

bool GridFillIter::next() {
	ERR_FAIL_COND_V_MSG(seen_id != current_seen_id, false, "Creating a new GridFillIter invalidate the previous one");

	while (!next_queue.empty()) {
		Vector2i seen_coord = next_queue.back();
		next_queue.pop();

		current = ChunkLocalCoord(seen_coord - Vector2i(256, 256) + start);

		Chunk *chunk = Grid::get_chunk(current.chunk_coord);
		if (chunk == nullptr) {
			continue;
		}

		u32 cell = chunk->get_cell(current.local_coord);
		if (Cell::material_idx(cell) != filter_material_idx) {
			continue;
		}

		// Add neighbors to queue.
		const Vector2i neighbors[4] = {
			Vector2i(0, -1),
			Vector2i(-1, 0),
			Vector2i(1, 0),
			Vector2i(0, 1),
		};
		for (u32 i = 0; i < 4; i++) {
			Vector2i new_seen_coord = seen_coord + neighbors[i];
			if (new_seen_coord.x < 0 ||
					new_seen_coord.x >= 512 ||
					new_seen_coord.y < 0 ||
					new_seen_coord.y >= 512) {
				// Bound reached
				continue;
			}

			if (seen[new_seen_coord.y * 512 + new_seen_coord.x] == current_seen_id) {
				// Already seen
				continue;
			}
			seen[new_seen_coord.y * 512 + new_seen_coord.x] = current_seen_id;

			next_queue.push(new_seen_coord);
		}

		return true;
	}

	return false;
}

u32 GridFillIter::get_material_idx() {
	return Grid::get_cell_material_idx(current);
}

u32 GridFillIter::get_color() {
	return Grid::get_cell_color(current);
}

void GridFillIter::set_material_idx(u32 material_idx) {
	Grid::set_cell_material_idx(current, material_idx);
}

void GridFillIter::set_color(u32 color) {
	Grid::set_cell_color(current, color);
}

void GridFillIter::fill_remaining(u32 material_idx) {
	while (next()) {
		set_material_idx(material_idx);
	}
}

Vector2i GridFillIter::chunk_coord() {
	return current.chunk_coord;
}

Vector2i GridFillIter::chunk_local_coord() {
	return current.local_coord;
}

Vector2i GridFillIter::coord() {
	return current.coord();
}

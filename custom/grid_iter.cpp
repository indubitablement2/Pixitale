#include "grid_iter.h"
#include "cell.hpp"
#include "core/math/rect2.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/os/memory.h"
#include "grid.h"
#include "preludes.h"

void GridChunkIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridChunkIter::next);

	ClassDB::bind_method(D_METHOD("set_cell", "value"), &GridChunkIter::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &GridChunkIter::get_cell);

	ClassDB::bind_method(D_METHOD("fill_remaining", "value"), &GridChunkIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("reset_iter"), &GridChunkIter::reset_iter);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridChunkIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("local_coord"), &GridChunkIter::local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridChunkIter::coord);

	ClassDB::bind_method(D_METHOD("randb"), &GridChunkIter::randb);
	ClassDB::bind_method(D_METHOD("randb_probability", "probability"), &GridChunkIter::randb_probability);
	ClassDB::bind_method(D_METHOD("randf"), &GridChunkIter::randf);
	ClassDB::bind_method(D_METHOD("randf_range", "min", "max"), &GridChunkIter::randf_range);
	ClassDB::bind_method(D_METHOD("randi_range", "min", "max"), &GridChunkIter::randi_range);
}

bool GridChunkIter::next() {
	return cell_iter.next();
}

void GridChunkIter::set_cell(u32 value) {
	if (chunk == nullptr) {
		return;
	}
	chunk->set_cell(cell_iter.coord, value);
}

u32 GridChunkIter::get_cell() {
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get_cell(cell_iter.coord);
}

void GridChunkIter::fill_remaining(u32 value) {
	while (next()) {
		set_cell(value);
	}
}

void GridChunkIter::reset_iter() {
	cell_iter = Iter2D();
}

Vector2i GridChunkIter::chunk_coord() {
	return _chunk_coord;
}

Vector2i GridChunkIter::local_coord() {
	return cell_iter.coord;
}

Vector2i GridChunkIter::coord() {
	return chunk_coord() * 32 + cell_iter.coord;
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

GridChunkIter::GridChunkIter(Vector2i chunk_coord, bool p_activate_on_destructor) {
	activate_on_destructor = p_activate_on_destructor;
	cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
	_chunk_coord = chunk_coord;
	chunk = Grid::get_chunk(chunk_coord);
	rng = Grid::get_static_rng(chunk_coord);
}

GridChunkIter::~GridChunkIter() {
	if (activate_on_destructor) {
		if (chunk != nullptr) {
			IterChunk _chunk_iter = IterChunk(Rect2(
					_chunk_coord * 32 - Vector2i(1, 1),
					Vector2i(34, 34)));

			while (_chunk_iter.next()) {
				Chunk *c = Grid::get_chunk(_chunk_iter.chunk_coord);
				if (c == nullptr) {
					continue;
				}

				c->activate_rect(_chunk_iter.local_rect());

				Iter2D _cell_iter = _chunk_iter.local_iter();
				while (_cell_iter.next()) {
					u32 cell = c->get_cell(_cell_iter.coord);
					Cell::set_active(cell, true);
					c->set_cell(_cell_iter.coord, cell);
				}
			}
		}
	}
}

void GridRectIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridRectIter::next);

	ClassDB::bind_method(D_METHOD("set_cell", "value"), &GridRectIter::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &GridRectIter::get_cell);

	ClassDB::bind_method(D_METHOD("fill_remaining", "value"), &GridRectIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("reset_iter"), &GridRectIter::reset_iter);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridRectIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("local_coord"), &GridRectIter::local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridRectIter::coord);
}

bool GridRectIter::next() {
	if (cell_iter.next()) {
		return true;
	} else {
		while (chunk_iter.next()) {
			chunk = Grid::get_chunk(chunk_iter.chunk_coord);
			if (chunk == nullptr) {
				continue;
			}

			cell_iter = chunk_iter.local_iter();
			if (cell_iter.next()) {
				return true;
			}
		}

		return false;
	}
}

void GridRectIter::set_cell(u32 value) {
	if (chunk == nullptr) {
		return;
	}
	chunk->set_cell(cell_iter.coord, value);
	modified = true;
}

u32 GridRectIter::get_cell() {
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get_cell(cell_iter.coord);
}

void GridRectIter::fill_remaining(u32 value) {
	while (next()) {
		set_cell(value);
	}
}

void GridRectIter::reset_iter() {
	chunk_iter = IterChunk(chunk_iter._start.coord(), chunk_iter._end.coord());
	cell_iter = Iter2D();
	chunk = nullptr;
}

Vector2i GridRectIter::chunk_coord() {
	return chunk_iter.chunk_coord;
}

Vector2i GridRectIter::local_coord() {
	return cell_iter.coord;
}

Vector2i GridRectIter::coord() {
	return chunk_coord() * 32 + cell_iter.coord;
}

GridRectIter::GridRectIter(Rect2i rect) {
	chunk_iter = IterChunk(rect);
	cell_iter = Iter2D();
	chunk = nullptr;
}

GridRectIter::~GridRectIter() {
	if (chunk != nullptr && modified) {
		IterChunk _chunk_iter = IterChunk(
				chunk_iter._start.coord() - Vector2i(1, 1),
				chunk_iter._end.coord() + Vector2i(1, 1));

		while (_chunk_iter.next()) {
			Chunk *c = Grid::get_chunk(_chunk_iter.chunk_coord);
			if (c == nullptr) {
				continue;
			}

			c->activate_rect(_chunk_iter.local_rect());

			Iter2D _cell_iter = _chunk_iter.local_iter();
			while (_cell_iter.next()) {
				u32 cell = c->get_cell(_cell_iter.coord);
				Cell::set_active(cell, true);
				c->set_cell(_cell_iter.coord, cell);
			}
		}
	}
}
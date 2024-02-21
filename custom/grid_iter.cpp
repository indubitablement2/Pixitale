#include "grid_iter.h"
#include "cell.hpp"
#include "core/error/error_macros.h"
#include "core/math/rect2.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/string/print_string.h"
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
	if (chunk == nullptr) {
		is_valid = false;
	} else {
		is_valid = cell_iter.next();
	}

	return is_valid;
}

void GridChunkIter::set_cell(u32 value) {
	ERR_FAIL_COND_MSG(!is_valid, "next should return true before using iter");

	chunk->set_cell(cell_iter.coord, value);
	modified = true;
}

u32 GridChunkIter::get_cell() {
	ERR_FAIL_COND_V_MSG(!is_valid, 0, "next should return true before using iter");

	return chunk->get_cell(cell_iter.coord);
}

void GridChunkIter::fill_remaining(u32 value) {
	while (next()) {
		set_cell(value);
	}
}

void GridChunkIter::reset_iter() {
	cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
	is_valid = false;
}

Vector2i GridChunkIter::chunk_coord() {
	return _chunk_coord;
}

Vector2i GridChunkIter::local_coord() {
	if (is_valid) {
		return cell_iter.coord;
	} else {
		return Vector2i(0, 0);
	}
}

Vector2i GridChunkIter::coord() {
	return chunk_coord() * 32 + local_coord();
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

void GridChunkIter::set_chunk(Vector2i chunk_coord) {
	modified = false;
	is_valid = false;
	_chunk_coord = chunk_coord;
	cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
	chunk = Grid::get_chunk(chunk_coord);
	rng = Grid::get_static_rng(chunk_coord);
}

void GridChunkIter::activate() {
	if (modified) {
		IterChunk _chunk_iter = IterChunk(Rect2(
				_chunk_coord * 32 - Vector2i(1, 1),
				Vector2i(34, 34)));

		while (_chunk_iter.next()) {
			Chunk *c = Grid::get_chunk(_chunk_iter.chunk_coord);
			if (c == nullptr) {
				continue;
			}

			TEST_ASSERT(_chunk_iter.local_rect().has_area(), "local_rect has no area");
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
		is_valid = true;
	} else {
		while (chunk_iter.next()) {
			chunk = Grid::get_chunk(chunk_iter.chunk_coord);
			if (chunk == nullptr) {
				continue;
			}

			cell_iter = chunk_iter.local_iter();
			if (cell_iter.next()) {
				is_valid = true;
				return true;
			}
		}

		is_valid = false;
	}

	return is_valid;
}

void GridRectIter::set_cell(u32 value) {
	ERR_FAIL_COND_MSG(!is_valid, "next should return true before using iter");

	chunk->set_cell(local_coord(), value);
	modified = true;
}

u32 GridRectIter::get_cell() {
	ERR_FAIL_COND_V_MSG(!is_valid, 0, "next should return true before using iter");

	return chunk->get_cell(local_coord());
}

void GridRectIter::fill_remaining(u32 value) {
	while (next()) {
		set_cell(value);
	}
}

void GridRectIter::reset_iter() {
	chunk_iter.reset();
	cell_iter = Iter2D();
	chunk = nullptr;
	is_valid = false;
}

Vector2i GridRectIter::chunk_coord() {
	return chunk_iter.chunk_coord;
}

Vector2i GridRectIter::local_coord() {
	if (is_valid) {
		return cell_iter.coord;
	} else {
		return Vector2i(0, 0);
	}
}

Vector2i GridRectIter::coord() {
	return chunk_coord() * 32 + local_coord();
}

void GridRectIter::set_rect(Rect2i rect) {
	modified = false;
	is_valid = false;
	chunk_iter = IterChunk(rect);
	cell_iter = Iter2D();
	chunk = nullptr;
}

void GridRectIter::activate() {
	if (modified) {
		Vector2i start = chunk_iter._start.coord() - Vector2i(1, 1);
		IterChunk _chunk_iter = IterChunk(Rect2i(
				start,
				chunk_iter._end.coord() + Vector2i(1, 1) - start));

		while (_chunk_iter.next()) {
			Chunk *c = Grid::get_chunk(_chunk_iter.chunk_coord);
			if (c == nullptr) {
				continue;
			}

			if (!_chunk_iter.local_rect().has_area()) {
				print_line(_chunk_iter.local_rect());
				print_line(_chunk_iter._start.chunk_coord);
				print_line(_chunk_iter._start.local_coord);
				print_line(_chunk_iter._end.chunk_coord);
				print_line(_chunk_iter._end.local_coord);
			}

			TEST_ASSERT(_chunk_iter.local_rect().has_area(), "local_rect has no area");
			c->activate_rect(_chunk_iter.local_rect());

			Iter2D _cell_iter = _chunk_iter.local_iter();
			while (_cell_iter.next()) {
				TEST_ASSERT(_cell_iter.coord.x >= 0, "coord.x is negative");
				TEST_ASSERT(_cell_iter.coord.y >= 0, "coord.y is negative");
				TEST_ASSERT(_cell_iter.coord.x < 32, "coord.x is too large");
				TEST_ASSERT(_cell_iter.coord.y < 32, "coord.y is too large");
				u32 cell = c->get_cell(_cell_iter.coord);
				Cell::set_active(cell, true);
				c->set_cell(_cell_iter.coord, cell);
			}
		}
	}
}

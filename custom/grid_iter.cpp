#include "grid_iter.h"
#include "grid.h"
#include "preludes.h"

void GridIter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GridIter::next);

	ClassDB::bind_method(D_METHOD("set_cell", "value"), &GridIter::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &GridIter::get_cell);

	ClassDB::bind_method(D_METHOD("fill_remaining", "value"), &GridIter::fill_remaining);

	ClassDB::bind_method(D_METHOD("reset_iter"), &GridIter::reset_iter);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GridIter::chunk_coord);
	ClassDB::bind_method(D_METHOD("local_coord"), &GridIter::local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GridIter::coord);

	ClassDB::bind_method(D_METHOD("randb"), &GridIter::randb);
	ClassDB::bind_method(D_METHOD("randb_probability", "probability"), &GridIter::randb_probability);
	ClassDB::bind_method(D_METHOD("randf"), &GridIter::randf);
	ClassDB::bind_method(D_METHOD("randf_range", "min", "max"), &GridIter::randf_range);
	ClassDB::bind_method(D_METHOD("randi_range", "min", "max"), &GridIter::randi_range);

	// ADD_PROPERTY(PropertyInfo(Variant::INT, "cell"), "set_cell", "get_cell");
}

bool GridIter::next() {
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

void GridIter::set_cell(u32 value) {
	if (chunk == nullptr) {
		return;
	}
	chunk->set_cell(cell_iter.coord, value);
}

u32 GridIter::get_cell() {
	if (chunk == nullptr) {
		return 0;
	}
	return chunk->get_cell(cell_iter.coord);
}

void GridIter::fill_remaining(u32 value) {
	Iter2D fill_cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
	while (fill_cell_iter.next()) {
		set_cell(value);
	}
}

void GridIter::reset_iter() {
	cell_iter = Iter2D();
	chunk_iter.reset();
	chunk = nullptr;
}

Vector2i GridIter::chunk_coord() {
	return chunk_iter.chunk_coord;
}

Vector2i GridIter::local_coord() {
	return cell_iter.coord;
}

Vector2i GridIter::coord() {
	return chunk_coord() * 32 + cell_iter.coord;
}

bool GridIter::randb() {
	return rng.gen_bool();
}

bool GridIter::randb_probability(f32 probability) {
	return rng.gen_probability_f32(probability);
}

f32 GridIter::randf() {
	return rng.gen_f32();
}

f32 GridIter::randf_range(f32 min, f32 max) {
	return rng.gen_range_f32(min, max);
}

i32 GridIter::randi_range(i32 min, i32 max) {
	return rng.gen_range_i32(min, max);
}

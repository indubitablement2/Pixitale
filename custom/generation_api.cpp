#include "generation_api.h"
#include "grid.h"
#include "preludes.h"

void GenerationApi::_bind_methods() {
	ClassDB::bind_method(D_METHOD("next"), &GenerationApi::next);

	ClassDB::bind_method(D_METHOD("set_cell", "value"), &GenerationApi::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &GenerationApi::get_cell);

	ClassDB::bind_method(D_METHOD("fill", "value"), &GenerationApi::fill);

	ClassDB::bind_method(D_METHOD("reset_iter"), &GenerationApi::reset_iter);

	ClassDB::bind_method(D_METHOD("chunk_coord"), &GenerationApi::chunk_coord);
	ClassDB::bind_method(D_METHOD("local_coord"), &GenerationApi::local_coord);
	ClassDB::bind_method(D_METHOD("coord"), &GenerationApi::coord);

	ClassDB::bind_method(D_METHOD("randb"), &GenerationApi::randb);
	ClassDB::bind_method(D_METHOD("randb_probability", "probability"), &GenerationApi::randb_probability);
	ClassDB::bind_method(D_METHOD("randf"), &GenerationApi::randf);
	ClassDB::bind_method(D_METHOD("randf_range", "min", "max"), &GenerationApi::randf_range);
	ClassDB::bind_method(D_METHOD("randi_range", "min", "max"), &GenerationApi::randi_range);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "cell"), "set_cell", "get_cell");
}

void GenerationApi::init(Vector2i chunk_coord) {
	_chunk_coord = chunk_coord;
	chunk = Grid::get_chunk(chunk_coord);
	rng = Grid::get_static_rng(chunk_coord);
	cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
}

bool GenerationApi::next() {
	return cell_iter.next();
}

void GenerationApi::set_cell(u32 value) {
	chunk->set_cell(cell_iter.coord, value);
}

u32 GenerationApi::get_cell() {
	return chunk->get_cell(cell_iter.coord);
}

void GenerationApi::fill(u32 value) {
	Iter2D fill_cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
	while (fill_cell_iter.next()) {
		set_cell(value);
	}
}

void GenerationApi::reset_iter() {
	cell_iter = Iter2D(Vector2i(0, 0), Vector2i(32, 32));
}

Vector2i GenerationApi::chunk_coord() {
	return _chunk_coord;
}

Vector2i GenerationApi::local_coord() {
	return cell_iter.coord;
}

Vector2i GenerationApi::coord() {
	return _chunk_coord * 32 + cell_iter.coord;
}

bool GenerationApi::randb() {
	return rng.gen_bool();
}

bool GenerationApi::randb_probability(f32 probability) {
	return rng.gen_probability_f32(probability);
}

f32 GenerationApi::randf() {
	return rng.gen_f32();
}

f32 GenerationApi::randf_range(f32 min, f32 max) {
	return rng.gen_range_f32(min, max);
}

i32 GenerationApi::randi_range(i32 min, i32 max) {
	return rng.gen_range_i32(min, max);
}

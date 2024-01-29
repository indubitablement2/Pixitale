#ifndef GRID_ITER_H
#define GRID_ITER_H

#include "chunk.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "preludes.h"
#include "rng.hpp"

class GridIter : public Object {
	GDCLASS(GridIter, Object);

protected:
	static void _bind_methods();

public:
	IterChunk chunk_iter;
	Iter2D cell_iter;
	Chunk *chunk;
	Rng rng;

	bool next();

	void set_cell(u32 value);
	u32 get_cell();

	void fill_remaining(u32 value);

	void reset_iter();

	Vector2i chunk_coord();
	Vector2i local_coord();
	Vector2i coord();

	bool randb();
	bool randb_probability(f32 probability);
	f32 randf();
	f32 randf_range(f32 min, f32 max);
	i32 randi_range(i32 min, i32 max);

	// u32 get_cell_at(Vector2i coord);
};

#endif
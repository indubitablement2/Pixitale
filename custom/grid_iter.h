#ifndef GRID_ITER_H
#define GRID_ITER_H

#include "chunk.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "preludes.h"
#include "rng.hpp"

class GridChunkIter : public Object {
	GDCLASS(GridChunkIter, Object);

protected:
	static void _bind_methods();

public:
	bool modified = false;
	bool is_valid = false;

	Iter2D cell_iter;
	Vector2i _chunk_coord;
	Chunk *chunk;
	Rng rng;

	bool next();

	void set_material_idx(u32 material_idx);
	void set_color(Color color = Color(0.5f, 0.5f, 0.5f, 1.0f));

	u32 get_material_idx();
	Color get_color();

	void fill_remaining(u32 material_idx);

	void reset_iter();

	Vector2i chunk_coord();
	Vector2i local_coord();
	Vector2i coord();

	bool randb();
	bool randb_probability(f32 probability);
	f32 randf();
	f32 randf_range(f32 min, f32 max);
	i32 randi_range(i32 min, i32 max);

	void set_chunk(Vector2i chunk_coord);
	void activate();
};

class GridRectIter : public Object {
	GDCLASS(GridRectIter, Object);

protected:
	static void _bind_methods();

public:
	bool modified = false;
	bool is_valid = false;

	IterChunk chunk_iter;
	Iter2D cell_iter;
	Chunk *chunk;

	bool next();

	void set_material_idx(u32 material_idx);
	void set_color(Color color = Color(0.5f, 0.5f, 0.5f, 1.0f));

	u32 get_material_idx();
	Color get_color();

	void fill_remaining(u32 material_idx);

	void reset_iter();

	Vector2i chunk_coord();
	Vector2i local_coord();
	Vector2i coord();

	void set_rect(Rect2i rect);
	void activate();
};

class GridLineIter : public Object {
	GDCLASS(GridLineIter, Object);

protected:
	static void _bind_methods();

public:
	bool is_valid = false;

	Vector2i start;

	IterLine line_iter;

	Chunk *chunk;
	ChunkLocalCoord current;

	bool next();

	void set_material_idx(u32 material_idx);
	void set_color(Color color = Color(0.5f, 0.5f, 0.5f, 1.0f));

	u32 get_material_idx();
	Color get_color();

	void fill_remaining(u32 material_idx);

	void reset_iter();

	Vector2i coord();

	void set_line(Vector2i p_start, Vector2i end);
};

// todo: bitmap iter, circle iter, explosion iter

#endif
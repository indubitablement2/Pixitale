#ifndef GRID_ITER_H
#define GRID_ITER_H

#include "chunk.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "preludes.h"
#include "rng.hpp"

class GridChunkIter : public Object {
	GDCLASS(GridChunkIter, Object);

protected:
	static void _bind_methods();

public:
	i32 x;
	i32 y;
	Vector2i _chunk_coord;
	Chunk *chunk;
	Rng rng;

	void prepare(Vector2i p_chunk_coord);

	bool next();

	u32 get_material_idx();
	u32 get_color();
	void set_material_idx(u32 material_idx);
	void set_color(u32 color);

	void fill_remaining(u32 material_idx);

	Vector2i chunk_coord();
	Vector2i chunk_local_coord();
	Vector2i coord();

	void reset_iter();

	bool randb();
	bool randb_probability(f32 probability);
	f32 randf();
	f32 randf_range(f32 min, f32 max);
	i32 randi_range(i32 min, i32 max);
};

class GridRectIter : public RefCounted {
	GDCLASS(GridRectIter, RefCounted);

protected:
	static void _bind_methods();

public:
	Iter2D iter;
	ChunkLocalCoord current;

	void prepare(Rect2i rect);

	bool next();

	u32 get_material_idx();
	u32 get_color();
	void set_material_idx(u32 material_idx);
	void set_color(u32 color);

	void fill_remaining(u32 material_idx);

	Vector2i chunk_coord();
	Vector2i chunk_local_coord();
	Vector2i coord();
};

class GridLineIter : public RefCounted {
	GDCLASS(GridLineIter, RefCounted);

protected:
	static void _bind_methods();

public:
	Vector2i start;
	IterLine iter;
	ChunkLocalCoord current;

	void prepare(Vector2i p_start, Vector2i p_end);

	bool next();

	u32 get_material_idx();
	u32 get_color();
	void set_material_idx(u32 material_idx);
	void set_color(u32 color);

	void fill_remaining(u32 material_idx);

	Vector2i chunk_coord();
	Vector2i chunk_local_coord();
	Vector2i coord();
};

class GridFillIter : public RefCounted {
	GDCLASS(GridFillIter, Object);

protected:
	static void _bind_methods();

public:
	i32 seen_id;
	u32 filter_material_idx;

	Vector2i start;
	ChunkLocalCoord current;

	void prepare(u32 p_filter_material_idx, Vector2i p_start);

	bool next();

	u32 get_material_idx();
	u32 get_color();
	void set_material_idx(u32 material_idx);
	void set_color(u32 color);

	void fill_remaining(u32 material_idx);

	Vector2i chunk_coord();
	Vector2i chunk_local_coord();
	Vector2i coord();
};

// todo:
// // - bitmap
// - circle
// - circle contour
// // - explosion
// - dijkstra
// - rect line
// - circle line

#endif
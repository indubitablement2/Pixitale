#ifndef PIXITALE_GRID_H
#define PIXITALE_GRID_H

#include "cell_material.h"
#include "chunk.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "grid_iter.h"
#include "preludes.h"
#include "rng.hpp"
#include <unordered_map>
#include <vector>

class Grid : public Object {
	GDCLASS(Grid, Object);

protected:
	static void _bind_methods();

public:
	inline static i64 tick = 0;
	inline static u64 seed = 0;

	inline static std::unordered_map<u64, Chunk *> chunks = {};
	inline static std::unordered_map<u64, u32> active_chunks = {};

	inline static std::vector<GridChunkIter *> chunk_iters = {};
	inline static std::vector<GridRectIter *> rect_iters = {};

	inline static std::vector<Rect2i> queue_step_chunk_rects = {};

	inline static Rng temporal_rng = Rng(0);

	// 3 passes of columns
	// x : ys (ys may be duplicated and aren't sorted)
	inline static std::unordered_map<i32, std::vector<i32>> passes[3] = { {}, {}, {} };

	inline static void clear_iters();
	static void clear();

	static void set_tick(i64 value);
	static i64 get_tick();

	static void set_seed(u64 value);
	static u64 get_seed();

	// Unaffected by time. Meant for world generation.
	static Rng get_static_rng(Vector2i chunk_coord);
	static Rng get_temporal_rng(Vector2i chunk_coord);

	static u64 chunk_id(Vector2i chunk_coord);
	static Chunk *get_chunk(Vector2i chunk_coord);

	// Return a fallback value if cell is not found.
	static u32 get_cell_material_idx(Vector2i coord);
	static CellMaterial *get_cell_material(Vector2i coord);

	static Rect2i get_chunk_active_rect(Vector2i chunk_coord);

	static void set_cell_rect(Rect2i rect, u32 material_idx);

	static Ref<Image> get_cell_buffer(Rect2i rect);

	static GridChunkIter *iter_chunk(Vector2i chunk_coord, bool activate);
	static GridRectIter *iter_rect(Rect2i rect);

	static void queue_step_chunks(Rect2i chunk_rect);
	// Part of step which can't be done async.
	static void step_prepare();
	static void step_start();
	static void step_wait_to_finish();

	bool randb();
	bool randb_probability(f32 probability);
	f32 randf();
	f32 randf_range(f32 min, f32 max);
	i32 randi_range(i32 min, i32 max);
};

#endif
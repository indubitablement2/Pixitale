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
	inline static u64 tick = 0;
	inline static u64 seed = 0;

	inline static std::unordered_map<u64, Chunk *> chunks = {};
	inline static std::unordered_map<u64, u32> active_chunks = {};

	inline static std::vector<GridIter *> iters = {};

	// void activate_neighbors(i32 x, i32 y, u32 *cell_ptr);

	// void set_cell_rect(Rect2i rect, u32 cell_material_idx);
	// void set_cell(Vector2i position, u32 cell_material_idx);
	// void set_cell_color(Vector2i position, u32 hue_palette_idx, u32 value_palette_idx);

	// static void set_biomes(Array biomes);

	static void clear();

	static void set_tick(u64 value);
	static u64 get_tick();

	static void set_seed(u64 value);
	static u64 get_seed();

	// Unaffected by time. Meant for world generation.
	static Rng get_static_rng(Vector2i chunk_coord);
	static Rng get_temporal_rng(Vector2i chunk_coord);

	static u64 chunk_id(Vector2i chunk_coord);
	static Chunk *get_chunk(Vector2i chunk_coord);

	// Return a fallback value if cell is not found.
	static u32 get_cell_material_idx(Vector2i coord);
	static Ref<CellMaterial> get_cell_material(Vector2i coord);

	static Rect2i get_chunk_active_rect(Vector2i chunk_coord);

	static void set_cell_rect(Rect2i rect, u32 material_idx);

	static Ref<Image> get_cell_buffer(Rect2i rect);

	static GridIter *iter(Rect2i rect);
	static GridIter *iter_chunk(Vector2i chunk_coord);

	static void step();
};

#endif
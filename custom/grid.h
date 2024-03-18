#ifndef GRID_H
#define GRID_H

#include "cell_material.hpp"
#include "chunk.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/variant/callable.h"
#include "grid_iter.h"
#include "preludes.h"
#include "rng.hpp"
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

const i32 GENERATION_SLICE_CHUNK_SIZE = 1024;

class Grid : public Object {
	GDCLASS(Grid, Object);

protected:
	static void _bind_methods();

private:
	// Key is lower material_idx | higher material_idx << 16.
	inline static std::unordered_map<u32, std::vector<CellReaction>> cell_reactions = {};

	inline static Callable generate_chunk_callback = Callable();
	inline static Callable generate_slice_callback = Callable();
	inline static std::unordered_set<i32> generated_slice = {};

	inline static i64 tick = 0;
	inline static u64 seed = 0;

	inline static std::unordered_map<u64, Chunk *> chunks = {};

	inline static std::vector<Rect2i> queue_step_chunk_rects = {};

	// 3 passes of columns
	// x : ys (ys may be duplicated and aren't sorted)
	inline static std::unordered_map<i32, std::vector<i32>> passes[3] = { {}, {}, {} };

public:
	inline static Rng temporal_rng = Rng(0);

	inline static std::vector<CellMaterial> cell_materials = {};

	// Any chunk last stepped before this will need to be force stepped.
	inline static i64 last_modified_tick = 0;

	// Current bitmask of updated cells.
	inline static u32 cell_updated_bitmask = 0;

	static std::vector<std::pair<Callable *, Vector2i>> &get_reaction_callback_vector();

	// Set pointers to nullptr if no reaction between m1 and m2.
	static void reactions_between(
			CellReaction *&start,
			CellReaction *&end,
			u32 m1,
			u32 m2,
			bool &swap);

	static void generate_chunk(Vector2i chunk_coord);

	// Return default if not found.
	static const CellMaterial &get_cell_material(u32 material_idx);

	// Unaffected by time. Meant for world generation.
	static Rng get_static_rng(Vector2i chunk_coord);
	static Rng get_temporal_rng(Vector2i chunk_coord);

	static u64 chunk_id(Vector2i chunk_coord);
	// Return nullptr if not found.
	static Chunk *get_chunk(Vector2i chunk_coord);

public: // godot api
	static void clear_cell_materials();
	static void add_cell_material(Object *obj);

	static void clear_cell_reactions();
	static u64 add_cell_reaction(
			u32 in1,
			u32 in2,
			u32 out1,
			u32 out2,
			f64 probability,
			Callable callback);
	static bool remove_cell_reaction(u64 reaction_id);
	static void print_internals();

	static void set_generate_chunk_callback(Callable value);
	static void set_generate_slice_callback(Callable value);

	static void clear();

	static void set_tick(i64 value);
	static i64 get_tick();

	static void set_seed(u64 value);
	static u64 get_seed();

	static Rect2i get_chunk_active_rect(Vector2i chunk_coord);

	static Ref<Image> get_cell_buffer(Rect2i chunk_rect, bool background);

	static u32 get_cell_data(ChunkLocalCoord coord);
	static u32 get_cell_material_idx(ChunkLocalCoord coord);
	static u32 get_cell_color(ChunkLocalCoord coord);
	static void set_cell_material_idx(ChunkLocalCoord coord, u32 material_idx);
	static void set_cell_color(ChunkLocalCoord coord, u32 color);

	static u32 get_cell_data_v(Vector2i coord);
	static u32 get_cell_material_idx_v(Vector2i coord);
	static u32 get_cell_color_v(Vector2i coord);
	static void set_cell_material_idx_v(Vector2i coord, u32 material_idx);
	static void set_cell_color_v(Vector2i coord, u32 color);

	static Ref<GridRectIter> iter_rect(Rect2i rect);
	static Ref<GridLineIter> iter_line(Vector2i start, Vector2i end);
	static Ref<GridFillIter> iter_fill(Vector2i start, u32 material_idx);

	static TypedArray<Vector2i> get_line(Vector2i start, Vector2i end);

	static void force_step();
	static void queue_step_chunks(Rect2i chunk_rect);
	// Part of step which can't be done async.
	static void step_prepare();
	static void step();

	static bool randb();
	static bool randb_probability(f32 probability);
	static f32 randf();
	static f32 randf_range(f32 min, f32 max);
	static i32 randi_range(i32 min, i32 max);

	static i64 div_floor(i64 numerator, i64 denominator);
	static Vector2i div_floor_v(Vector2i numerator, Vector2i denominator);
	static i64 mod_neg(i64 numerator, i64 denominator);
	static Vector2i mod_neg_v(Vector2i numerator, Vector2i denominator);
};

VARIANT_ENUM_CAST(CellCollision);

#endif
#ifndef GRID_H
#define GRID_H

#include "cell_material.hpp"
#include "chunk.h"
#include "core/io/image.h"
#include "core/math/color.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/variant/callable.h"
#include "core/variant/variant.h"
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

	// (iter: GridChunkIter, slice_idx: int)
	inline static Callable generate_chunk_callback = Callable();
	// (slice_idx: int)
	inline static Callable generate_slice_callback = Callable();
	inline static std::unordered_set<i32> generated_slice = {};
	// (chunk_coord: Vector2i)
	inline static Callable unload_chunk_callback = Callable();

	inline static i64 tick = 0;
	inline static u64 seed = 0;

	inline static std::unordered_map<u64, Chunk *> chunks = {};

	inline static std::vector<Rect2i> queue_step_chunk_rects = {};

public:
	// 3 passes of columns
	// x : ys (y may be duplicated and aren't sorted)
	inline static std::vector<std::pair<i32, std::vector<i32>>> passes[3] = { {}, {}, {} };
	inline static i32 current_pass_idx = 0;

	inline static Rng temporal_rng = Rng(0);

	inline static std::vector<CellMaterial> cell_materials = {};

	// Any chunk last stepped before this will need to be force stepped.
	inline static i64 last_modified_tick = 0;

	// Current bitmask for updated cells.
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

	static void set_callbacks(
			Callable generate_chunk,
			Callable generate_slice);

	static void clear();

	static void set_tick(i64 value);
	static i64 get_tick();

	static void set_seed(u64 value);
	static u64 get_seed();

	static Rect2i get_chunk_active_rect(Vector2i chunk_coord);
	static i64 get_grid_memory_usage();

	static Ref<Image> get_cell_buffer(Rect2i rect, bool background, bool clean);

	static PackedByteArray get_chunk_state(Vector2i chunk_coord);

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

	static Ref<GridChunkIter> iter_chunk(Vector2i chunk_coord);
	static Ref<GridRectIter> iter_rect(Rect2i rect);
	static Ref<GridLineIter> iter_line(Vector2i start, Vector2i end);
	static Ref<GridFillIter> iter_fill(Vector2i start, u32 material_idx);

	static TypedArray<Vector2i> get_line(Vector2i start, Vector2i end);

	static bool chunk_exists(Vector2i chunk_coord);

	static bool try_create_chunk(Vector2i chunk_coord);
	static void step_chunk(Vector2i chunk_coord);
	static void pre_step();
	static void post_step();

	static bool randb();
	static bool randb_probability(f32 probability);
	static f32 randf();
	static f32 randf_range(f32 min, f32 max);
	static i32 randi_range(i32 min, i32 max);

	static u32 color_to_material_idx(Color color);
	static u32 color_to_color_idx(Color color);

	static i64 _div_floor(i64 numerator, i64 denominator);
	static Vector2i _div_floor_v(Vector2i numerator, Vector2i denominator);
	static i64 _mod_neg(i64 numerator, i64 denominator);
	static Vector2i _mod_neg_v(Vector2i numerator, Vector2i denominator);
};

VARIANT_ENUM_CAST(CellCollision);

#endif
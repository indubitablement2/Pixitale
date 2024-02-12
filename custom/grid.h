#ifndef GRID_H
#define GRID_H

#include "cell_material.h"
#include "chunk.h"
#include "core/io/image.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/variant/callable.h"
#include "generation_pass.h"
#include "grid_iter.h"
#include "preludes.h"
#include "rng.hpp"
#include <unordered_map>
#include <vector>

class Grid : public Object {
	GDCLASS(Grid, Object);

protected:
	static void _bind_methods();

private:
	inline static std::vector<CellMaterial> cell_materials = {};

	// Key is lower material_idx | higher material_idx << 16.
	inline static std::unordered_map<u32, std::vector<CellReaction>> cell_reactions = {};

	inline static std::vector<GenerationPass *> generation_passes = {};

	inline static i64 tick = 0;
	inline static u64 seed = 0;

	inline static std::unordered_map<u64, Chunk *> chunks = {};
	inline static std::unordered_map<u64, u32> active_chunks = {};

	inline static std::vector<GridChunkIter *> chunk_iters = {};
	inline static std::vector<GridRectIter *> rect_iters = {};

	inline static std::vector<Rect2i> queue_step_chunk_rects = {};

	inline static Rng temporal_rng = Rng(0);

	inline static bool force_step = false;

	// 3 passes of columns
	// x : ys (ys may be duplicated and aren't sorted)
	inline static std::unordered_map<i32, std::vector<i32>> passes[3] = { {}, {}, {} };

	inline static void clear_iters();

public:
	// Any chunk last stepped before this will need to be force stepped.
	inline static i64 last_modified_tick = 0;

	// Set pointers to nullptr if no reaction between m1 and m2.
	static void reactions_between(
			CellReaction *&start,
			CellReaction *&end,
			u32 m1,
			u32 m2,
			bool &swap);

	static void generate_chunk(Vector2i chunk_coord);

	inline static bool is_force_step() { return force_step; }

	// Return default if not found.
	static CellMaterial &get_cell_material(u32 material_idx);

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
	static u64 add_cell_reaction(u32 in1, u32 in2, u32 out1, u32 out2, f64 probability);
	static bool remove_cell_reaction(u64 reaction_id);

	static void clear_generation_passes();
	static void add_generation_pass(GenerationPass *value);

	static void clear();

	static void set_tick(i64 value);
	static i64 get_tick();

	static void set_seed(u64 value);
	static u64 get_seed();

	// Return a fallback value if cell is not found.
	static u32 get_cell_material_idx(Vector2i coord);

	static Rect2i get_chunk_active_rect(Vector2i chunk_coord);

	static Ref<Image> get_cell_buffer(Rect2i chunk_rect);

	static GridChunkIter *iter_chunk(Vector2i chunk_coord);
	static GridRectIter *iter_rect(Rect2i rect);

	static void set_force_step(bool value);
	static void queue_step_chunks(Rect2i chunk_rect);
	// Part of step which can't be done async.
	static void step_prepare();
	static void step_start();
	static void step_wait_to_finish();

	static bool randb();
	static bool randb_probability(f32 probability);
	static f32 randf();
	static f32 randf_range(f32 min, f32 max);
	static i32 randi_range(i32 min, i32 max);
};

VARIANT_ENUM_CAST(CellCollision);

#endif
#ifndef GRID_H
#define GRID_H

#include "preludes.h"

#include "core/io/image.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

struct CellReaction {
	// chance/2^32 - 1.
	u32 probability;
	// If eq in1 does not change material.
	u32 mat_idx_out1;
	// If eq in2 does not change material.
	u32 mat_idx_out2;
};

class CellMaterial {
public:
	// StringName display_name;
	// color: ();

	int cell_movement;
	int density;

	float durability;

	int cell_collision;
	float friction;

	int reaction_ranges_len;
	// Has all reactions with material that have idx >= this material's idx.
	uint64_t *reaction_ranges;
	CellReaction *reactions;

	// on_destroyed: ();

	CellMaterial() :
			cell_movement(0),
			density(0),
			durability(0),
			cell_collision(0),
			friction(0),
			reaction_ranges_len(0),
			reaction_ranges(nullptr),
			reactions(nullptr){};
};

class Grid : public Object {
	GDCLASS(Grid, Object);

protected:
	static void _bind_methods();

public:
	inline static u32 *cells = nullptr;
	inline static i32 width = 0;
	inline static i32 height = 0;

	// Same height as cells. Width is one chunk (32).
	inline static u32 *border_cells = nullptr;

	inline static u64 *chunks = nullptr;
	inline static i32 chunks_width = 0;
	inline static i32 chunks_height = 0;

	inline static i64 tick = 0;
	inline static u32 updated_bit = 0;

	inline static CellMaterial *cell_materials = nullptr;
	inline static i32 cell_materials_len = 0;

	inline static u64 seed = 0;

	enum CellMovement {
		CELL_MOVEMENT_SOLID,
		CELL_MOVEMENT_POWDER,
		CELL_MOVEMENT_LIQUID,
		CELL_MOVEMENT_GAS,
	};

	enum CellCollision {
		CELL_COLLISION_NONE,
		CELL_COLLISION_SOLID,
		CELL_COLLISION_PLATFORM,
		CELL_COLLISION_LIQUID,
	};

	static void delete_grid();
	static void new_empty(i32 wish_width, i32 wish_height);
	static Vector2i get_size();
	static Vector2i get_size_chunk();

	static Ref<Image> get_cell_data(Vector2i image_size, Rect2i rect);
	// Return fallback cell if out of bounds.
	static u32 get_cell_checked(i32 x, i32 y);

	static void activate_neighbors(i32 x, i32 y, u32 *cell_ptr);
	static void set_cell_rect(Rect2i rect, u32 cell_material_idx);
	static void set_cell(Vector2i position, u32 cell_material_idx);
	static void set_border_cell(Vector2i position, u32 cell_material_idx);

	static void step_manual();

	static void init_materials(i32 num_materials);
	static void add_material(
			int cell_movement,
			int density,
			int durability,
			int cell_collision,
			float friction,
			// probability, out1, out2
			Array reactions,
			int idx);

	static i64 get_tick();
	static u32 get_cell_material_idx(Vector2i position);
	static bool is_chunk_active(Vector2i position);

	static void set_seed(i64 new_seed);
	static i64 get_seed();

	static void free_memory();
	static void print_materials();
	static void run_tests();
};

VARIANT_ENUM_CAST(Grid::CellMovement);
VARIANT_ENUM_CAST(Grid::CellCollision);

#endif
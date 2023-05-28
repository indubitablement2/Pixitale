#ifndef GRID_H
#define GRID_H

#include "core/io/image.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "godot/core/typedefs.h"

struct CellReaction {
	// chance/2^32 - 1.
	uint32_t probability;
	// If eq in1 does not change material.
	uint32_t mat_idx_out1;
	// If eq in2 does not change material.
	uint32_t mat_idx_out2;
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
	// Row major.
	inline static uint32_t *cells = nullptr;
	inline static uint32_t width = 0;
	inline static uint32_t height = 0;

	// Row major. Same height as cells. Width is one chunk (32).
	inline static uint32_t *border_cells = nullptr;

	// TODO: Store chunks row major as we can update per row instead.
	// Column major.
	inline static uint64_t *chunks = nullptr;
	inline static uint32_t chunks_width = 0;
	inline static uint32_t chunks_height = 0;

	inline static int64_t tick = 0;
	inline static uint32_t updated_bit = 0;

	inline static CellMaterial *cell_materials = nullptr;
	inline static uint32_t cell_materials_len = 0;

	inline static uint64_t seed = 0;

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
	static void new_empty(uint32_t wish_width, uint32_t wish_height);
	static Vector2i get_size();
	static Vector2i get_size_chunk();

	static Ref<Image> get_cell_data(Vector2i image_size, Rect2i rect);
	// Return fallback cell if out of bounds.
	static uint32_t get_cell_checked(uint32_t x, uint32_t y);

	static void set_cell_rect(Rect2i rect, uint32_t cell_material_idx);
	static void set_cell(Vector2i position, uint32_t cell_material_idx);
	static void set_border_cell(Vector2i position, uint32_t cell_material_idx);

	static void step_manual();

	static void init_materials(uint32_t num_materials);
	static void add_material(
			int cell_movement,
			int density,
			int durability,
			int cell_collision,
			float friction,
			// probability, out1, out2
			Array reactions,
			int idx);

	static int64_t get_tick();
	static uint32_t get_cell_material_idx(Vector2i position);
	static bool is_chunk_active(Vector2i position);

	static void set_seed(int64_t new_seed);
	static int64_t get_seed();

	static void free_memory();
	static void print_materials();
	static void run_tests();
};

VARIANT_ENUM_CAST(Grid::CellMovement);
VARIANT_ENUM_CAST(Grid::CellCollision);

#endif
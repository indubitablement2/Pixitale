#ifndef GRID_H
#define GRID_H

#include "preludes.h"

#include "cell_material.h"
#include "core/io/image.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include <cell.hpp>

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

	inline static u64 tick = 0;

	inline static std::vector<CellMaterial> m = {};

	// inline static

	inline static u64 seed = 0;

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
	static void set_cell_color(Vector2i position, u32 hue_palette_idx, u32 value_palette_idx);

	static void take_border_cells();
	static void set_cell_generation(Vector2i position, u32 cell_material_idx);
	static void post_generation_pass();

	// TODO: Rename to step as no other step exists.
	static void step_manual();

	static void add_material(
			i32 movement,
			i32 density,
			f32 durability,
			i32 collision,
			f32 friction,
			bool can_color,
			u32 max_value_noise,
			Ref<Image> values,
			Array reactions);

	static i64 get_tick();
	static u32 get_cell_material_idx(Vector2i position);
	static bool is_chunk_active(Vector2i position);

	static void set_seed(i64 new_seed);
	static i64 get_seed();

	static void free_memory();
	static void print_materials();
	static void run_tests();
};

// VARIANT_ENUM_CAST(Grid::CellMovement);
// VARIANT_ENUM_CAST(Grid::CellCollision);

VARIANT_ENUM_CAST(Cell::Movement);
VARIANT_ENUM_CAST(Cell::Collision);

#endif
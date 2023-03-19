#ifndef GRID_H
#define GRID_H

#include <bit>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>

using namespace godot;

typedef uint32_t cell_t;
typedef uint64_t chunk_t;

struct ChunkActiveRect {
    int column_skip;
    int column_count;
    // Stating from the top.
    int row_skip;
    int row_count;
};

struct CellMaterial {
    StringName display_name;
    // color: ();
    int movement;
    float density;

    float durability;

    int collision;
    float friction;
    float bounciness;

    // on_destroyed: ();

    // Has all reactions with material that have idx >= this material's idx.
    // reactions_range: Vec<u64>;
    // reactions: Vec<Reaction>;
};

struct Reaction {
    float probability;
    // Out of bound when reaction does not change material.
    int material_out1;
    // Out of bound when reaction does not change material.
    int material_out2;
};

class Grid : public Node2D {
    GDCLASS(Grid, Node2D);

protected:
    static void _bind_methods();

private:
	inline static cell_t* _cells = nullptr;
    inline static int _width = 0;
    inline static int _height = 0;

    inline static chunk_t* _chunks = nullptr;
    inline static int _chunk_width = 0;
    inline static int _chunk_height = 0;

    inline static cell_t _update_bit = 0;

    enum CellShifts {
        CELL_SHIFT_UPDATED = 12,
        CELL_SHIFT_ACTIVE = 14,
        CELL_SHIFT_COLOR = 24,
    };

    enum CellMasks {
		CELL_MASK_MATERIAL = 0xFFF,
        // Alternate between 1, 2 and 3. 
        // 0 used for inactive/new cell. eg. always update.
        CELL_MASK_UPDATED = 0b11 << CellShifts::CELL_SHIFT_UPDATED,
        CELL_MASK_ACTIVE = 1 << CellShifts::CELL_SHIFT_ACTIVE,
        CELL_MASK_COLOR = 0xFF << CellShifts::CELL_SHIFT_COLOR,
	};

    static cell_t cell_material_idx(cell_t cell);
    static void cell_set_material_idx(cell_t& cell, cell_t material_idx);
    static bool cell_is_updated(cell_t cell);
    static void cell_set_updated(cell_t& cell);
    static bool cell_is_active(cell_t cell);
    static void cell_set_active(cell_t& cell, bool active);
    // uint8_t color(cell_t cell);
    // void set_color(cell_t& cell, uint8_t color);

    static void chunk_set_active(int x, int y);
    static bool chunk_is_row_inactive(chunk_t chunk, int row);
    static ChunkActiveRect chunk_active_rect(chunk_t chunk);
public:
    inline const static float GRID_SCALE = 4.0f;

    enum CellMovement {
        CELL_MOVEMENT_SOLID,
        CELL_MOVEMENT_POWDER,
        CELL_MOVEMENT_LIQUID,
        CELL_MOVEMENT_GAS,
    };

    enum CellCollision {
        CELL_COLLISION_SOLID,
        CELL_COLLISION_PLATFORM,
        CELL_COLLISION_LIQUID,
        CELL_COLLISION_NONE,
    };

    static void delete_grid();
    static void new_empty(int width, int height);
    static Vector2i get_size();
    static void draw_rect(Rect2i rect, CanvasItem *on, Vector2i at);
    static void set_texture_data(Ref<ImageTexture> texture, Rect2i rect);
};

VARIANT_ENUM_CAST(Grid::CellMovement);
VARIANT_ENUM_CAST(Grid::CellCollision);

#endif
#ifndef GRID_H
#define GRID_H

#include <bit>
#include <map>
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

struct CellReaction {
    // chance/2^32 - 1.
    uint32_t probability;
    // TODO: if eq do not change instead.
    // Out of bound (UINT32_MAX) when reaction does not change material.
    cell_t mat_idx_out1;
    // Out of bound (UINT32_MAX) when reaction does not change material.
    cell_t mat_idx_out2;
};

struct CellMaterial {
    StringName display_name;
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
};

class Grid : public Object {
    GDCLASS(Grid, Object);

protected:
    static void _bind_methods();

private:
	inline static cell_t *_cells = nullptr;
    inline static int _width = 0;
    inline static int _height = 0;

    inline static chunk_t *_chunks = nullptr;
    inline static int _chunk_width = 0;
    inline static int _chunk_height = 0;

    inline static cell_t _update_bit = 0;
    inline static uint64_t _tick = 0;

    inline static CellMaterial *_materials = nullptr;
    inline static int _materials_len = 0;

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

    static void set_area_active(int x, int y, cell_t *center_ptr);
    static void chunk_set_active(int x, int y);
    static bool chunk_is_row_inactive(chunk_t chunk, int row);
    static ChunkActiveRect chunk_active_rect(chunk_t chunk);

    static void step_column(int column_idx);
    static void step_cell(int x, int y, uint64_t &rng);
    static int step_reaction(
        cell_t mat1_idx,
        CellMaterial *mat1,
        cell_t mat2_idx,
        CellMaterial *mat2,
        uint64_t &rng,
        cell_t &out1,
        cell_t &out2
    );

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
    static void step_manual();

    static void delete_materials();
    static void init_materials(int num_materials);
    static void add_material(
        // StringName display_name;
        // color: ();
        int cell_movement,
        int density,

        int durability,

        int cell_collision,
        float friction,

        // probability, out1, out2
        Array reactions,

        // int reaction_ranges_len;
        // // Has all reactions with material that have idx >= this material's idx.
        // uint64_t *reaction_ranges;
        // CellReaction *reactions;

        // on_destroyed: ();
        
        int idx
    );

    static void free_memory();

    static void print_materials();
};

// struct CellReactionBuilder {
//     StringName id1;
//     StringName id2;
//     uint32_t probability;
//     StringName id_out1;
//     StringName id_out2;
// };

// class MaterialsBuilder : public RefCounted {
//     GDCLASS(MaterialsBuilder, RefCounted);

// protected:
//     static void _bind_methods();

// private:
//     std::map<StringName, CellMaterial> _materials = {};
//     std::map<StringName, std::vector<CellReactionBuilder>> _reactions = {};
//     std::map<StringName, std::vector<StringName>> _tags = {};

// public:
//     void add_material(
//         StringName id,
//         StringName display_name,
//         int cell_movement,
//         int density,

//         int durability,

//         int cell_collision,
//         float friction,
//         float bounciness
//     );
//     void add_reaction(
//         StringName id1,
//         StringName id2,
//         double probability,
//         StringName id_out1,
//         StringName id_out2
//     );
//     void build();
// };

VARIANT_ENUM_CAST(Grid::CellMovement);
VARIANT_ENUM_CAST(Grid::CellCollision);

#endif
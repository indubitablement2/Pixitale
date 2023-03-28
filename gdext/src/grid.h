#ifndef GRID_H
#define GRID_H

#include <cstdint>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include "chunk.h"
#include "cell.h"

using namespace godot;

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
    inline static CellMaterial *materials = nullptr;
    inline static int materials_len = 0;

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
        reactions(nullptr)
    {};
};

class Grid : public Object {
    GDCLASS(Grid, Object);

protected:
    static void _bind_methods();

private:
    // void set_cell(
    //     int x,
    //     int y,
    //     uint32_t *cell,
    //     bool active,
    //     bool changed
    // );

    static void step_column(int column_idx); 
    static void step_chunk(
        Chunk *chunk,
        uint32_t *cell_start,
        uint32_t rows,
        ChunkActiveRect rect,
        uint32_t &rng
    );
    static void step_cell(CellApi &cell, uint32_t &rng);
    static void step_reaction(CellApi &cell, bool &active, bool &changed, CellApi &other, uint32_t &rng);

public:
    inline static uint32_t *cells = nullptr;
    inline static int width = 0;
    inline static int height = 0;
    
    inline static Chunk *chunks = nullptr;
    inline static int chunks_width = 0;
    inline static int chunks_height = 0;

    inline static int64_t tick = 0;

    inline const static float GRID_SCALE = 4.0f;

    inline static int count = 0;

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
    static void new_empty(int wish_width, int wish_height);
    static Vector2i get_size();
    static Vector2i get_size_chunk();
    static void update_texture_data(Ref<ImageTexture> texture, Vector2i position);
    static void step_manual();

    static void init_materials(int num_materials);
    static void add_material(
        int cell_movement,
        int density,
        int durability,
        int cell_collision,
        float friction,
        // probability, out1, out2
        Array reactions,
        int idx
    );

    static int64_t get_tick();
    static uint32_t get_cell_material_idx(Vector2i position);
    static bool is_chunk_active(Vector2i position);

    static void free_memory();
    static void print_materials();
};

VARIANT_ENUM_CAST(Grid::CellMovement);
VARIANT_ENUM_CAST(Grid::CellCollision);

#endif
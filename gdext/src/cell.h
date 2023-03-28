#ifndef CELL_H
#define CELL_H

#include <godot_cpp/godot.hpp>
#include "chunk.h"

using namespace godot;

// All methods assume grid is infinite.
class CellApi {
private:
    enum Shifts {
        SHIFT_UPDATED = 12,
        SHIFT_ACTIVE = 14,
        SHIFT_COLOR = 24,
    };

    enum Masks {
		MASK_MATERIAL = 0xFFF,
        // Alternate between 1, 2 and 3. 
        // 0 used for inactive/new cell. eg. always update.
        MASK_UPDATED = 0b11 << Shifts::SHIFT_UPDATED,
        MASK_ACTIVE = 1 << Shifts::SHIFT_ACTIVE,
        MASK_COLOR = 0xFF << Shifts::SHIFT_COLOR,
	};

public:
    inline static uint32_t updated_bit = 0;

    Chunk *chunk;
    uint32_t *cell;
    int local_x;
    int local_y;

    CellApi(){};
    CellApi(Chunk *chunk, uint32_t *cell, int local_x, int local_y)
        : chunk(chunk), cell(cell), local_x(local_x), local_y(local_y) {};

    void offset_same_chunk(int x, int y);

    void left();
    void up();
    void right();
    void down();

    uint32_t material_idx();
    void set_material_idx(uint32_t material_idx);
    bool is_updated();
    void set_updated();
    bool is_active();
    // Set cell and chunk to active.
    // When inactive, set updated bit to 0 (never skip).
    void set_active(bool active);
    // Set cells in a 3x3 area to active including itself.
    void set_area_active();

    static void step_updated_bit();

    static uint32_t cell_material_idx(uint32_t cell);
    static void cell_set_active(uint32_t &cell, bool active);
};

#endif
#include "cell.h"
#include "chunk.h"
#include "godot_cpp/variant/utility_functions.hpp"
#include "grid.h"

void CellApi::offset_same_chunk(int x, int y) {
    local_x += x;
    local_y += y;
    cell += x + (y * Grid::width);
}

void CellApi::left() {
    if (local_x == 0) {
        chunk--;
        local_x = 31;
        cell--;
    } else {
        offset_same_chunk(-1, 0);
    }
}

void CellApi::up() {
    if (local_y == 0) {
        chunk -= Grid::chunks_width;
        local_y = 31;
        cell -= Grid::width;
    } else {
        offset_same_chunk(0, -1);
    }
}

void CellApi::right() {
    if (local_x == 31) {
        chunk++;
        local_x = 0;
        cell++;
    } else {
        offset_same_chunk(1, 0);
    }
}

void CellApi::down() {
    if (local_y == 31) {
        chunk += Grid::chunks_width;
        local_y = 0;
        cell += Grid::width;
    } else {
        offset_same_chunk(0, 1);
    }
}

uint32_t CellApi::material_idx() {
    return *cell & Masks::MASK_MATERIAL;
}

void CellApi::set_material_idx(uint32_t material_idx) {
    *cell = (*cell & ~Masks::MASK_MATERIAL) | material_idx;
}

bool CellApi::is_updated() {
    return (*cell & Masks::MASK_UPDATED) == updated_bit;
}

void CellApi::set_updated() {
    *cell = (*cell & ~Masks::MASK_UPDATED) | updated_bit;
}

bool CellApi::is_active() {
    return *cell & Masks::MASK_ACTIVE;
}

void CellApi::set_active(bool active) {
    if (active) {
        chunk->set_active(local_x, local_y);
        *cell |= Masks::MASK_ACTIVE;
    } else {
        *cell &= ~Masks::MASK_ACTIVE;
        *cell &= ~Masks::MASK_UPDATED;
    }
}

void CellApi::set_area_active() {
    CellApi cell = *this;
    cell.set_active(true);
    cell.right();
    cell.set_active(true);
    cell.down();
    cell.set_active(true);
    cell.left();
    cell.set_active(true);
    cell.left();
    cell.set_active(true);
    cell.up();
    cell.set_active(true);
    cell.up();
    cell.set_active(true);
    cell.right();
    cell.set_active(true);
    cell.right();
    cell.set_active(true);
}

void CellApi::step_updated_bit() {
    updated_bit = (((updated_bit >> Shifts::SHIFT_UPDATED) % 3) + 1) << Shifts::SHIFT_UPDATED;
}

uint32_t CellApi::cell_material_idx(uint32_t cell) {
    return cell & Masks::MASK_MATERIAL;
}

void CellApi::cell_set_active(uint32_t &cell, bool active) {
    if (active) {
        cell |= Masks::MASK_ACTIVE;
    } else {
        cell &= ~Masks::MASK_ACTIVE;
        cell &= ~Masks::MASK_UPDATED;
    }
}
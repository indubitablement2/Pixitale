#include <bit>
#include "grid.h"

void Grid::_bind_methods() {
    ClassDB::bind_static_method("Grid", D_METHOD("delete_grid"), &Grid::delete_grid);
    ClassDB::bind_static_method("Grid", D_METHOD("new_empty", "width", "height"), &Grid::new_empty);

    // ADD_GROUP("Test group", "group_");
	// ADD_SUBGROUP("Test subgroup", "group_subgroup_");

    // Add the enums.
    BIND_ENUM_CONSTANT(CELL_COLLISION_SOLID);
    BIND_ENUM_CONSTANT(CELL_COLLISION_PLATFORM);
    BIND_ENUM_CONSTANT(CELL_COLLISION_LIQUID);
    BIND_ENUM_CONSTANT(CELL_COLLISION_NONE);

    BIND_ENUM_CONSTANT(CELL_MOVEMENT_SOLID);
    BIND_ENUM_CONSTANT(CELL_MOVEMENT_POWDER);
    BIND_ENUM_CONSTANT(CELL_MOVEMENT_LIQUID);
    BIND_ENUM_CONSTANT(CELL_MOVEMENT_GAS);
}

void Grid::delete_grid() {
    if (_cells != nullptr) {
        delete[] _cells;
    }
}

void Grid::new_empty(int width, int height) {
    delete_grid();

    _chunk_width = std::max(width / 32, 3);
    // Make sure it is a multiple of 8,
    // so that cache lines are not shared between threads.
    _chunk_height = (std::max(height / 32, 3) + 7) & ~7;
    _chunks = new chunk_t[_chunk_width * _chunk_height];

    _width = _chunk_width * 32;
    _height = _chunk_height * 32;
    _cells = new cell_t[_width * _height];
}

cell_t Grid::cell_material_idx(cell_t cell) {
    return cell & CellMasks::MATERIAL_MASK;
}

void Grid::cell_set_material_idx(cell_t& cell, cell_t material_idx) {
    cell = (cell & ~CellMasks::MATERIAL_MASK) | material_idx;
}

bool Grid::cell_is_updated(cell_t cell) {
    return (cell & CellMasks::UPDATED_MASK) == _update_bit;
}

void Grid::cell_set_updated(cell_t& cell) {
    cell = (cell & ~CellMasks::UPDATED_MASK) | _update_bit;
}

bool Grid::cell_is_active(cell_t cell) {
    return (cell & CellMasks::ACTIVE_MASK) != 0;
}

void Grid::cell_set_active(cell_t& cell, bool active) {
    if (active) {
        cell |= CellMasks::ACTIVE_MASK;
    } else {
        cell &= ~CellMasks::ACTIVE_MASK;
    }
}

void Grid::chunk_set_active(int x, int y) {
    // int idx = (y >> 5) + (x >> 5) * _chunk_width;
    // chunk_t bitmask = (1 << ((x & 31) + 32)) | (1 << (y & 31));
    // chunk_t* chunk = _chunks + idx;
    // *chunk |= bitmask;
}

bool Grid::chunk_is_row_inactive(chunk_t chunk, int row) {
    return (chunk & (1 << row)) == 0;
}

ChunkActiveRect Grid::chunk_active_rect(chunk_t chunk) {
    ChunkActiveRect rect;
    rect.column_skip = 0;
    rect.column_count = 0;
    rect.row_skip = 0;
    rect.row_count = 0;

    if (chunk == 0) {
        return rect;
    }

    uint cols = uint(chunk >> 32);
    uint rows = uint(chunk);

    rect.column_skip = std::countr_zero(cols);
    rect.column_count = 32 - std::countl_zero(cols) - rect.column_skip;
    rect.row_skip = std::countr_zero(rows);
    rect.row_count = 32 - std::countl_zero(rows) - rect.row_skip;

    return rect;
}
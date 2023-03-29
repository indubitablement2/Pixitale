#include <bit>
#include "chunk.h"

void Chunk::set_inactive() {
    this->rows = 0;
    this->columns = 0;
}

void Chunk::set_active(const int local_x, const int local_y) {
    this->rows |= 1 << local_y;
    this->columns |= 1 << local_x;
}

bool Chunk::is_active() {
    return this->rows;
}

ChunkActiveRect Chunk::active_rect() {
    ChunkActiveRect rect;

    if (!is_active()) {
        rect.x_start = 0;
        rect.x_end = 0;
        rect.y_start = 0;
        rect.y_end = 0;
        return rect;
    }

    rect.x_start = std::countr_zero(columns);
    rect.x_end = 32 - std::countl_zero(columns);
    rect.y_start = std::countr_zero(rows);
    rect.y_end = 32 - std::countl_zero(rows);

    return rect;
}

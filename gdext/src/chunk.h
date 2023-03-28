#ifndef CHUNK_H
#define CHUNK_H

#include <godot_cpp/godot.hpp>

using namespace godot;

struct ChunkActiveRect {
    int x_start;
    int x_end;
    int y_start;
    int y_end;
};

class alignas(64) Chunk {
public:
    uint32_t rows;
    uint32_t columns;

    // TODO: compute and cache biome when chunk becomes inactive
    // // For cell spreading and quick biome.
    // int num_non_empty_cell;

    // Start fully active by default.
    Chunk() : rows(0xffffffff), columns(0xffffffff) {};

    void set_inactive();
    // Does not set cell to active.
    void set_active(const int local_x, const int local_y);
    bool is_active();
    ChunkActiveRect active_rect();
};

// Avoid sharing cache lines between thread.
static_assert(sizeof(Chunk) == 64, "Chunk size is not 64 bytes");

#endif
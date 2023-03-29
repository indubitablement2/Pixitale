#include <algorithm>
#include <cstdint>
#include <cstring>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image_texture.hpp>

// #define NDEBUG 
#include "assert.h"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/vector2i.hpp"
#include "grid.h"
#include "chunk.h"

namespace Cell {

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

inline uint32_t material_idx(const uint32_t &cell) {
    return cell & Masks::MASK_MATERIAL;
}

inline void set_material_idx(uint32_t &cell, const uint32_t material_idx) {
    cell = (cell & ~Masks::MASK_MATERIAL) | material_idx;
}

inline bool is_updated(const uint32_t &cell) {
    return (cell & Masks::MASK_UPDATED) == updated_bit;
}

inline void set_updated(uint32_t &cell) {
    cell = (cell & ~Masks::MASK_UPDATED) | updated_bit;
}

inline bool is_active(const uint32_t &cell) {
    return cell & Masks::MASK_ACTIVE;
}

// When inactive, set updated bit to 0 (never skip when it return to active).
inline void set_active(uint32_t &cell, const bool active) {
    if (active) {
        cell |= Masks::MASK_ACTIVE;
    } else {
        cell &= ~Masks::MASK_ACTIVE;
        cell &= ~Masks::MASK_UPDATED;
    }
}

} // namespace Cell

// Rect needs to be within a single chunk.
void activate_rect(Chunk *chunk, int x_offset, int y_offset, uint64_t width, uint64_t height) {
    assert(x_offset >= 0 && y_offset >= 0 && x_offset + width <= 32 && y_offset + height <= 32);

    chunk->rows |= ((1 << height) - 1) << y_offset;
    chunk->columns |= ((1 << width) - 1) << x_offset; // TODO: +32
}

void activate_point(Chunk *chunk, int x, int y) {
    activate_rect(chunk, x, y, 1, 1);
}

void activate_neightbors(Chunk *chunk, int local_x, int local_y, uint32_t *cell) {
    if (local_x <= 0 && local_y <= 0) {
        // Top left corner
        activate_rect(chunk, 0, 0, 2, 2);
        activate_rect(chunk - 1, 31, 0, 1, 2);
        activate_rect(chunk - 1 - chunks_width, 31, 31, 1, 1);
        activate_rect(chunk - chunks_width, 0, 31, 2, 1);
    } else if (local_x <= 0 && local_y >= 31) {
        // Bottom left corner
        activate_rect(chunk, 0, 30, 2, 2);
        activate_rect(chunk - 1, 31, 30, 1, 2);
        activate_rect(chunk - 1 + chunks_width, 31, 0, 1, 1);
        activate_rect(chunk + chunks_width, 0, 0, 2, 1);
    } else if (local_x >= 31 && local_y <= 0) {
        // Top right corner
        activate_rect(chunk, 30, 0, 2, 2);
        activate_rect(chunk + 1, 0, 0, 1, 2);
        activate_rect(chunk + 1 - chunks_width, 0, 31, 1, 1);
        activate_rect(chunk - chunks_width, 30, 31, 2, 1);
    } else if (local_x >= 31 && local_y >= 31) {
        // Bottom right corner
        activate_rect(chunk, 30, 30, 2, 2);
        activate_rect(chunk + 1, 0, 30, 1, 2);
        activate_rect(chunk + 1 + chunks_width, 0, 0, 1, 1);
        activate_rect(chunk + chunks_width, 30, 0, 2, 1);
    } else if (local_x <= 0) {
        // Left edge
        activate_rect(chunk, 0, local_y - 1, 2, 3);
        activate_rect(chunk - 1, 31, local_y - 1, 1, 3);
    } else if (local_y <= 0) {
        // Top edge
        activate_rect(chunk, local_x - 1, 0, 3, 2);
        activate_rect(chunk - chunks_width, local_x - 1, 31, 3, 1);
    } else if (local_x >= 31) {
        // Right edge
        activate_rect(chunk, 30, local_y - 1, 2, 3);
        activate_rect(chunk + 1, 0, local_y - 1, 1, 3);
    } else if (local_y >= 31) {
        // Bottom edge
        activate_rect(chunk, local_x - 1, 30, 3, 2);
        activate_rect(chunk + chunks_width, local_x - 1, 0, 3, 1);
    } else {
        // Middle
        activate_rect(chunk, local_x - 1, local_y - 1, 3, 3);
    }

    Cell::set_active(*(cell - 1), true);
    Cell::set_active(*cell, true);
    Cell::set_active(*(cell + 1), true);
    Cell::set_active(*(cell - width - 1), true);
    Cell::set_active(*(cell - width), true);
    Cell::set_active(*(cell - width + 1), true);
    Cell::set_active(*(cell + width - 1), true);
    Cell::set_active(*(cell + width), true);
    Cell::set_active(*(cell + width + 1), true);
}

// Only works for offset up to 32.
void offset_chunk_local_position(Chunk *&chunk, int &local_x, int &local_y, int offset_x, int offset_y) {
    assert(offset_x <= 32 && offset_y <= 32);
    
    local_x += offset_x;
    local_y += offset_y;

    if (local_x < 0) {
        chunk -= 1;
        local_x += 32;
    } else if (local_x >= 32) {
        chunk += 1;
        local_x -= 32;
    }

    if (local_y < 0) {
        chunk -= chunks_width;
        local_y += 32;
    } else if (local_y >= 32) {
        chunk += chunks_width;
        local_y -= 32;
    }
}

namespace Step {

void step_reaction(
    uint32_t &cell_material_idx,
    bool &active,
    bool &changed,
    Chunk *chunk,
    int local_x,
    int local_y,
    uint32_t *other_ptr,
    int other_offset_x,
    int other_offset_y,
    uint32_t &rng
) {
    auto other_material_idx = Cell::material_idx(*other_ptr);
    
    bool swap;
    CellMaterial *mat;
    uint32_t reaction_range_idx;
    if (cell_material_idx > other_material_idx) {
        swap = true;
        mat = cell_materials + other_material_idx;
        reaction_range_idx = cell_material_idx - other_material_idx;
    } else {
        swap = false;
        mat = cell_materials + cell_material_idx;
        reaction_range_idx = other_material_idx - cell_material_idx;
    }

    if (reaction_range_idx >= mat->reaction_ranges_len) {
        return;
    }

    uint64_t reaction_range = *(mat->reaction_ranges + reaction_range_idx);
    uint64_t reaction_start = reaction_range & 0xffffffff;
    uint64_t reaction_end = reaction_range >> 32;

    if (reaction_start >= reaction_end) {
        return;
    }

    active = true;

    for (uint64_t i = reaction_start; i < reaction_end; i++) {
        CellReaction reaction = *(mat->reactions + i);

        rng = rng * 1284865807 + 4150755601;
        if (reaction.probability >= rng) {
            uint32_t out1, out2;
            if (swap) {
                out1 = reaction.mat_idx_out2;
                out2 = reaction.mat_idx_out1;
            } else {
                out1 = reaction.mat_idx_out1;
                out2 = reaction.mat_idx_out2;
            }
            
            if (out1 != cell_material_idx) {
                cell_material_idx = out1;
                changed = true;
            }

            if (out2 != other_material_idx) {
                Cell::set_material_idx(*other_ptr, out2);
                auto other_chunk = chunk;
                auto other_local_x = local_x;
                auto other_local_y = local_y;
                offset_chunk_local_position(
                    other_chunk,
                    other_local_x,
                    other_local_y,
                    other_offset_x,
                    other_offset_y
                );
                activate_neightbors(other_chunk, other_local_x, other_local_y, other_ptr);
            }

            return;
        }
    }

    count++;
}

void step_cell(
    uint32_t *cell_ptr,
    Chunk *chunk,
    int local_x,
    int local_y,
    uint32_t &rng
) {
    uint32_t cell = *cell_ptr;

    if (!Cell::is_active(cell) || Cell::is_updated(cell)) {
        return;
    }

    bool active = false;
    // Activate a 5x5 rect around the cell.
    bool changed = false;

    uint32_t cell_material_idx = Cell::material_idx(cell);

    // Reactions
    // x x x
    // . o x
    // . . .

    step_reaction(
        cell_material_idx,
        active,
        changed,
        chunk,
        local_x,
        local_y,
        cell_ptr + 1,
        1,
        0,
        rng
    );

    step_reaction(
        cell_material_idx,
        active,
        changed,
        chunk,
        local_x,
        local_y,
        cell_ptr - width - 1,
        -1,
        -1,
        rng
    );

    step_reaction(
        cell_material_idx,
        active,
        changed,
        chunk,
        local_x,
        local_y,
        cell_ptr - width,
        0,
        -1,
        rng
    );

    step_reaction(
        cell_material_idx,
        active,
        changed,
        chunk,
        local_x,
        local_y,
        cell_ptr - width + 1,
        1,
        -1,
        rng
    );

    // TODO: Movement

    if (changed) {
        Cell::set_material_idx(cell, cell_material_idx);
        Cell::set_updated(cell);
        *cell_ptr = cell;

        activate_neightbors(chunk, local_x, local_y, cell_ptr);
    } else if (active) {
        Cell::set_updated(cell);
        Cell::set_active(cell, true);
        *cell_ptr = cell;

        activate_point(chunk, local_x, local_y);
    } else {
        Cell::set_active(cell, false);
        *cell_ptr = cell;
    }
}

void step_chunk(
    Chunk *chunk,
    uint32_t *cell_start,
    uint32_t rows,
    ChunkActiveRect rect,
    uint32_t &rng
) {
    if (rows == 0) {
        return;
    }

    UtilityFunctions::print("x_start ", rect.x_start, " x_end ", rect.x_end, " y_start ", rect.y_start, " y_end ", rect.y_end);

    // Iterate over each cell in the chunk.
    for (int local_y = rect.y_end - 1; local_y >= rect.y_start; local_y--) {
        if ((rows & (1 << local_y)) == 0) {
            continue;
        }

        for (int local_x = rect.x_start; local_x < rect.x_end; local_x++) {
            auto cell_ptr = cell_start + local_x + local_y * width;
            step_cell(cell_ptr, chunk, local_x, local_y, rng);
        }
    }
}

void step_column(int column_idx) {
    uint32_t rng = (uint32_t)((int64_t)column_idx * tick * 6364136223846792969);

    Chunk *chunk_end = chunks + column_idx;
    Chunk *next_chunk = chunk_end + (chunks_height - 2) * chunks_width;

    auto next_chunk_rows = next_chunk->rows;
    auto next_chunk_rect = next_chunk->active_rect();
    next_chunk->set_inactive();
    
    auto cell_start = cells + ((height - 32) * width + column_idx * 32);

    // Iterate over each chunk from the bottom.
    while (next_chunk > chunk_end) {
        auto rows = next_chunk_rows;
        auto rect = next_chunk_rect;
        auto chunk = next_chunk;

        next_chunk -= chunks_width;
        next_chunk_rows = next_chunk->rows;
        next_chunk_rect = next_chunk->active_rect();
        next_chunk->set_inactive();

        cell_start -= 32 * width;

        step_chunk(chunk, cell_start, rows, rect, rng);
    }
}

void pre_step() {
    updated_bit = (((updated_bit >> Cell::Shifts::SHIFT_UPDATED) % 3) + 1) << Cell::Shifts::SHIFT_UPDATED;

    tick++;
}

} // namespace Grid

void Grid::_bind_methods() {
    ClassDB::bind_static_method(
        "Grid", 
        D_METHOD("delete_grid"),
        &Grid::delete_grid
    );
    ClassDB::bind_static_method(
        "Grid", 
        D_METHOD("new_empty", "width", "height"),
        &Grid::new_empty
    );
    ClassDB::bind_static_method(
        "Grid", 
        D_METHOD("get_size"),
        &Grid::get_size
    );
    ClassDB::bind_static_method(
        "Grid", 
        D_METHOD("get_size_chunk"),
        &Grid::get_size_chunk
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("update_texture_data", "texture", "position"),
        &Grid::update_texture_data
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("step_manual"),
        &Grid::step_manual
    );

    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("get_tick"),
        &Grid::get_tick
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("get_cell_material_idx", "position"),
        &Grid::get_cell_material_idx
    );

    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("init_materials", "num_materials"),
        &Grid::init_materials
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD(
            "add_material",
            "cell_movement",
            "density",
            "durability",
            "cell_collision",
            "friction",
            "reactions"
        ),
        &Grid::add_material
    );

    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("is_chunk_active", "position"),
        &Grid::is_chunk_active
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("free_memory"),
        &Grid::free_memory
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("print_materials"),
        &Grid::print_materials
    );
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("run_tests"),
        &Grid::run_tests
    );

    // ADD_GROUP("Test group", "group_");
	// ADD_SUBGROUP("Test subgroup", "group_subgroup_");

    BIND_CONSTANT(GRID_SCALE);

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
    if (cells != nullptr) {
        delete [] cells;
        cells = nullptr;
        width = 0;
        height = 0;

        delete [] chunks;
        chunks = nullptr;
        chunks_width = 0;
        chunks_height = 0;
    }
}

void Grid::new_empty(int wish_width, int wish_height) {
    delete_grid();

    chunks_width = std::max(wish_width / 32, 3);
    chunks_height = std::max(wish_height / 32, 3);
    chunks = new Chunk[chunks_width * chunks_height];

    width = chunks_width * 32;
    height = chunks_height * 32;
    cells = new uint32_t[width * height];

    // iterate over all cells
    for (int i = 0; i < height * width; i++) {
        auto cell_ptr = cells + i;
        uint32_t cell = i % 2;
        Cell::set_active(cell, true);
        *cell_ptr = cell;
    }
}

Vector2i Grid::get_size() {
    return Vector2i(width, height);
}

Vector2i Grid::get_size_chunk() {
    return Vector2i(chunks_width, chunks_height);
}

void Grid::update_texture_data(Ref<ImageTexture> texture, Vector2i position) {
    if (cells == nullptr) {
        UtilityFunctions::push_warning("Grid is not initialized");
        return;
    }

    int texture_width = texture->get_width();
    int texture_height = texture->get_height();

    if (texture_width == 0 || texture_height == 0) {
        UtilityFunctions::push_warning("Texture has zero size");
        return;
    }

    int skip_texture_left = std::max(0, -position.x);
    int skip_texture_top = std::max(0, -position.y);
    int texture_write_width = std::min(texture_width, width - skip_texture_left);
    int texture_write_height = std::min(texture_height, height - skip_texture_top);

    int skip_cell_left = std::max(0, position.x) * sizeof(uint32_t);
    int skip_cell_top = std::max(0, position.y);

    texture_width *= sizeof(uint32_t);
    skip_texture_left *= sizeof(uint32_t);
    texture_write_width *= sizeof(uint32_t);

    // TODO: Reuse this buffer.
    PackedByteArray data = PackedByteArray();
    data.resize(texture_width * texture_height);
    auto data_ptr = data.ptrw() + skip_texture_left + skip_texture_top * texture_width;

    auto cell_ptr = reinterpret_cast<uint8_t*>(cells + skip_cell_left + skip_cell_top * width * sizeof(uint32_t));

    // Copy each rows.
    for (int y = 0; y < texture_write_height; y++) {
        memcpy(
            data_ptr,
            cell_ptr,
            texture_write_width
        );
        data_ptr += texture_width;
        cell_ptr += width * sizeof(uint32_t);
    }

    Ref<Image> image = Image::create_from_data(
        texture_width / sizeof(uint32_t),
        texture_height,
        false,
        Image::FORMAT_RGBA8,
        data
    );

    texture->update(image);
}

void Grid::step_manual() {
    if (cells == nullptr) {
        UtilityFunctions::push_warning("Grid is not initialized");
        return;
    }

    Step::pre_step();

    count = 0;

    for (int column_idx = 1; column_idx < chunks_width - 1; column_idx++) {
        Step::step_column(column_idx);
    }

    UtilityFunctions::print("count: ", count);
}

void delete_materials() {
    if (cell_materials != nullptr) {
        for (int i = 0; i < cell_materials_len; i++) {
            CellMaterial *mat = cell_materials + i;
            if (mat->reaction_ranges_len > 0) {
                delete[] mat->reaction_ranges;
                delete[] mat->reactions;
            }
        }

        delete[] cell_materials;
        cell_materials = nullptr;
        cell_materials_len = 0;
    }
}

void Grid::init_materials(int num_materials) {
    delete_materials();

    if (num_materials > 0) {
        cell_materials = new CellMaterial[num_materials];
        cell_materials_len = num_materials;
    }
}

void Grid::add_material(
    int cell_movement,
    int density,
    int durability,
    int cell_collision,
    float friction,
    // probability, out1, out2
    Array reactions,
    int idx
) {
    if (cell_materials == nullptr) {
        UtilityFunctions::push_error("Materials not initialized");
        return;
    }

    CellMaterial mat = CellMaterial();

    int num_reaction = 0;
    for (int i = 0; i < reactions.size(); i++) {
        Array r = reactions[i];
        if (!r.is_empty()) {
            mat.reaction_ranges_len = i + 1;
            num_reaction += r.size();
        }
    }
    if (mat.reaction_ranges_len != 0) {
        mat.reaction_ranges = new uint64_t[mat.reaction_ranges_len];
        mat.reactions = new CellReaction[num_reaction];

        uint64_t next_reaction_idx = 0;
        for (int i = 0; i < reactions.size(); i++) {
            uint64_t reactions_start = next_reaction_idx;

            Array reactions_with = reactions[i];
            for (int j = 0; j < reactions_with.size(); j++) {
                Array reaction_data = reactions_with[j];
                CellReaction reaction = {
                    reaction_data[0],
                    reaction_data[1],
                    reaction_data[2]
                };
                mat.reactions[next_reaction_idx] = reaction;

                next_reaction_idx++;
            }

            uint64_t reactions_end = next_reaction_idx;

            if (reactions_start == reactions_end) {
                mat.reaction_ranges[i] = 0;
            } else {
                mat.reaction_ranges[i] = reactions_start | (reactions_end << 32);
            }
        }

        if (next_reaction_idx != num_reaction) {
            UtilityFunctions::push_error("Invalid reaction ranges, c++ logic error");
        }
    }

    cell_materials[idx] = mat;
}

bool Grid::is_chunk_active(Vector2i position) {
    if (position.x < 0 || position.y < 0 || position.x >= width || position.y >= height) {
        return false;
    }

    return (chunks + position.x + position.y * chunks_width)->is_active();
}

void Grid::free_memory() {
    delete_materials();
    delete_grid();
}

int64_t Grid::get_tick() {
    return tick;
}

uint32_t Grid::get_cell_material_idx(Vector2i position) {
    if (position.x < 0 || position.y < 0 || position.x >= width || position.y >= height) {
        return 0;
    }

    return Cell::material_idx(cells[position.x + position.y * width]);
}

void Grid::print_materials() {
    UtilityFunctions::print("num materials: ", cell_materials_len);

    for (int i = 0; i < cell_materials_len; i++) {
        CellMaterial &mat = cell_materials[i];
        UtilityFunctions::print("-----------", i, "-----------");
        UtilityFunctions::print("cell_movement ", mat.cell_movement);
        UtilityFunctions::print("density ", mat.density);
        UtilityFunctions::print("durability ", mat.durability);
        UtilityFunctions::print("cell_collision ", mat.cell_collision);
        UtilityFunctions::print("friction ", mat.friction);

        UtilityFunctions::print("reaction_ranges_len ", mat.reaction_ranges_len);
        for (int j = 0; j < mat.reaction_ranges_len; j++) {
            UtilityFunctions::print("   reaction_range ", j);
            uint64_t reaction_range = *(mat.reaction_ranges + j);
            uint64_t reaction_start = reaction_range & 0xffffffff;
            uint64_t reaction_end = reaction_range >> 32;
            UtilityFunctions::print("       reaction_start ", reaction_start);
            UtilityFunctions::print("       reaction_end ", reaction_end);
            for (int k = reaction_start; k < reaction_end; k++) {
                CellReaction reaction = *(mat.reactions + k);
                UtilityFunctions::print("          reaction ", k);
                UtilityFunctions::print("          in1 ", i);
                UtilityFunctions::print("          in2 ", i + j);
                UtilityFunctions::print("          probability ", reaction.probability);
                UtilityFunctions::print("          out1 ", reaction.mat_idx_out1);
                UtilityFunctions::print("          out2 ", reaction.mat_idx_out2);
            }
        }
    }
}

namespace Test {

void activate_chunk() {
    Grid::new_empty(96, 96);
    auto chunk = chunks + 1 + chunks_width;
    auto cell_ptr = cells + 32 + 32 * width;

    activate_neightbors(chunk, 15, 15, cell_ptr + 15 + 15 * width);
    UtilityFunctions::print("activate center: OK");

    activate_neightbors(chunk, 0, 0, cell_ptr);
    UtilityFunctions::print("activate top left: OK");

    activate_neightbors(chunk, 31, 31, cell_ptr + 31 + 31 * width);
    UtilityFunctions::print("activate bottom right: OK");

    activate_neightbors(chunk, 31, 0, cell_ptr + 31);
    UtilityFunctions::print("activate top right: OK");

    activate_neightbors(chunk, 0, 31, cell_ptr + 31 * width);
    UtilityFunctions::print("activate bottom left: OK");

    activate_neightbors(chunk, 15, 0, cell_ptr + 15);
    UtilityFunctions::print("activate top center: OK");

    activate_neightbors(chunk, 15, 31, cell_ptr + 15 + 31 * width);
    UtilityFunctions::print("activate bottom center: OK");
    
    activate_neightbors(chunk, 0, 15, cell_ptr + 15 * width);
    UtilityFunctions::print("activate left center: OK");

    activate_neightbors(chunk, 31, 15, cell_ptr + 15 + 31 * width);
    UtilityFunctions::print("activate right center: OK");

    Grid::delete_grid();
}

} // namespace Test

void Grid::run_tests() {
    Test::activate_chunk();
    UtilityFunctions::print("activate_chunk: OK");

    UtilityFunctions::print("All tests passed");
}

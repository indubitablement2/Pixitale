// #define NDEBUG 
#include <assert.h>

#include <bit>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include "grid.h"

using namespace godot;

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

namespace Chunk {

struct ChunkActiveRect {
    int x_start;
    int x_end;
    int y_start;
    int y_end;
};

uint32_t get_rows(uint64_t chunk) {
    return (uint32_t)chunk;
}

uint32_t get_columns(uint64_t chunk) {
    return (uint32_t)(chunk >> 32);
}

ChunkActiveRect active_rect(uint64_t chunk) {
    ChunkActiveRect rect;

    if (chunk == 0) {
        rect.x_start = 0;
        rect.x_end = 0;
        rect.y_start = 0;
        rect.y_end = 0;
        return rect;
    }

    uint32_t rows = get_rows(chunk);
    uint32_t columns = get_columns(chunk);

    assert(rows > 0);
    assert(columns > 0);

    rect.x_start = std::countr_zero(columns);
    rect.x_end = 32 - std::countl_zero(columns);
    rect.y_start = std::countr_zero(rows);
    rect.y_end = 32 - std::countl_zero(rows);

    return rect;
}

// Rect needs to be within a single chunk.
void activate_rect(uint64_t *chunk_ptr, uint64_t x_offset, uint64_t y_offset, uint64_t width, uint64_t height) {
    assert(x_offset >= 0);
    assert(y_offset >= 0);
    assert(x_offset < 32);
    assert(y_offset < 32);
    assert(x_offset + width <= 32);
    assert(y_offset + height <= 32);
    assert(width > 0);
    assert(height > 0);

    *chunk_ptr |= ((1uLL << height) - 1uLL) << y_offset; // Set rows
    *chunk_ptr |= ((1uLL << width) - 1uLL) << (x_offset + 32); // Set columns
    
    assert(get_rows(*chunk_ptr) > 0);
    assert(get_columns(*chunk_ptr) > 0);
}

void activate_point(uint64_t *chunk_ptr, int x, int y) {
    activate_rect(chunk_ptr, x, y, 1, 1);
}

// Unlike other functions, this one also activate the cells.
void activate_neightbors(uint64_t *chunk_ptr, int local_x, int local_y, uint32_t *cell) {
    if (local_x <= 0 && local_y <= 0) {
        // Top left corner
        activate_rect(chunk_ptr, 0, 0, 2, 2);
        activate_rect(chunk_ptr - chunks_height, 31, 0, 1, 2);
        activate_rect(chunk_ptr - chunks_height - 1, 31, 31, 1, 1);
        activate_rect(chunk_ptr - 1, 0, 31, 2, 1);
    } else if (local_x <= 0 && local_y >= 31) {
        // Bottom left corner
        activate_rect(chunk_ptr, 0, 30, 2, 2);
        activate_rect(chunk_ptr - chunks_height, 31, 30, 1, 2);
        activate_rect(chunk_ptr - chunks_height + 1, 31, 0, 1, 1);
        activate_rect(chunk_ptr + 1, 0, 0, 2, 1);
    } else if (local_x >= 31 && local_y <= 0) {
        // Top right corner
        activate_rect(chunk_ptr, 30, 0, 2, 2);
        activate_rect(chunk_ptr + chunks_height, 0, 0, 1, 2);
        activate_rect(chunk_ptr + chunks_height - 1, 0, 31, 1, 1);
        activate_rect(chunk_ptr - 1, 30, 31, 2, 1);
    } else if (local_x >= 31 && local_y >= 31) {
        // Bottom right corner
        activate_rect(chunk_ptr, 30, 30, 2, 2);
        activate_rect(chunk_ptr + chunks_height, 0, 30, 1, 2);
        activate_rect(chunk_ptr + chunks_height + 1, 0, 0, 1, 1);
        activate_rect(chunk_ptr + 1, 30, 0, 2, 1);
    } else if (local_x <= 0) {
        // Left edge
        activate_rect(chunk_ptr, 0, local_y - 1, 2, 3);
        activate_rect(chunk_ptr - chunks_height, 31, local_y - 1, 1, 3);
    } else if (local_y <= 0) {
        // Top edge
        activate_rect(chunk_ptr, local_x - 1, 0, 3, 2);
        activate_rect(chunk_ptr - 1, local_x - 1, 31, 3, 1);
    } else if (local_x >= 31) {
        // Right edge
        activate_rect(chunk_ptr, 30, local_y - 1, 2, 3);
        activate_rect(chunk_ptr + chunks_height, 0, local_y - 1, 1, 3);
    } else if (local_y >= 31) {
        // Bottom edge
        activate_rect(chunk_ptr, local_x - 1, 30, 3, 2);
        activate_rect(chunk_ptr + 1, local_x - 1, 0, 3, 1);
    } else {
        // Middle
        activate_rect(chunk_ptr, local_x - 1, local_y - 1, 3, 3);
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
void offset_chunk_local_position(
    uint64_t *&chunk_ptr,
    int &local_x,
    int &local_y,
    int offset_x,
    int offset_y
) {
    assert(offset_x <= 32);
    assert(offset_y <= 32);
    assert(offset_x >= -32);
    assert(offset_y >= -32);
    
    local_x += offset_x;
    local_y += offset_y;

    if (local_x < 0) {
        chunk_ptr -= chunks_height;
        local_x += 32;
    } else if (local_x >= 32) {
        chunk_ptr += chunks_height;
        local_x -= 32;
    }

    if (local_y < 0) {
        chunk_ptr -= 1;
        local_y += 32;
    } else if (local_y >= 32) {
        chunk_ptr += 1;
        local_y -= 32;
    }

    assert(local_x >= 0);
    assert(local_y >= 0);
    assert(local_x < 32);
    assert(local_y < 32);
}

}

namespace Step {

void step_reaction(
    uint32_t &cell_material_idx,
    bool &active,
    bool &changed,
    uint64_t *chunk_ptr,
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
                auto other_chunk_ptr = chunk_ptr;
                auto other_local_x = local_x;
                auto other_local_y = local_y;
                Chunk::offset_chunk_local_position(
                    other_chunk_ptr,
                    other_local_x,
                    other_local_y,
                    other_offset_x,
                    other_offset_y
                );
                Chunk::activate_neightbors(
                    other_chunk_ptr,
                    other_local_x,
                    other_local_y,
                    other_ptr
                );
            }

            return;
        }
    }
}

void step_cell(
    uint32_t *cell_ptr,
    uint64_t *chunk_ptr,
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
        chunk_ptr,
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
        chunk_ptr,
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
        chunk_ptr,
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
        chunk_ptr,
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

        Chunk::activate_neightbors(chunk_ptr, local_x, local_y, cell_ptr);
    } else if (active) {
        Cell::set_updated(cell);
        Cell::set_active(cell, true);
        *cell_ptr = cell;

        Chunk::activate_point(chunk_ptr, local_x, local_y);
    } else {
        Cell::set_active(cell, false);
        *cell_ptr = cell;
    }
}

void step_chunk(
    uint64_t chunk,
    uint64_t *chunk_ptr,
    uint32_t *cell_start,
    uint32_t &rng
) {
    if (chunk == 0) {
        return;
    }

    auto rows = Chunk::get_rows(chunk);
    auto rect = Chunk::active_rect(chunk);

    // Iterate over each cell in the chunk.
    for (int local_y = rect.y_end - 1; local_y >= rect.y_start; local_y--) {
        if ((rows & (1 << local_y)) == 0) {
            continue;
        }

        for (int local_x = rect.x_start; local_x < rect.x_end; local_x++) {
            auto cell_ptr = cell_start + local_x + local_y * width;
            step_cell(cell_ptr, chunk_ptr, local_x, local_y, rng);
        }
    }
}

void step_column(int column_idx) {
    uint32_t rng = (uint32_t)((int64_t)column_idx * tick * 6364136223846792969);

    uint64_t *chunk_end_ptr = chunks + column_idx * chunks_height;
    uint64_t *next_chunk_ptr = chunk_end_ptr + (chunks_height - 2);

    auto next_chunk = *next_chunk_ptr;
    *next_chunk_ptr = 0;
    
    auto cell_start = cells + ((height - 32) * width + column_idx * 32);

    // Iterate over each chunk from the bottom.
    while (next_chunk_ptr > chunk_end_ptr) {
        auto chunk = next_chunk;
        auto chunk_ptr = next_chunk_ptr;

        next_chunk_ptr -= 1;
        next_chunk = *next_chunk_ptr;
        *next_chunk_ptr = 0;

        cell_start -= 32 * width;

        step_chunk(chunk, chunk_ptr, cell_start, rng);
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
    chunks = new uint64_t[chunks_width * chunks_height];
    // Set all chunk to active.
    for (int i = 0; i < chunks_width * chunks_height; i++) {
        chunks[i] = 0xFFFFFFFFFFFFFFFF;
    }

    width = chunks_width * 32;
    height = chunks_height * 32;
    cells = new uint32_t[width * height];
    // Set all cells to empty.
    for (int i = 0; i < width * height; i++) {
        cells[i] = 0;
    }

    // iterate over all cells
    for (int x = 32; x < width - 32; x++) {
        for (int y = 32; y < height - 32; y++) {
            auto cell_ptr = cells + (y * width + x);
            uint32_t cell = x % 2;
            Cell::set_active(cell, true);
            *cell_ptr = cell;
        }
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

    // UtilityFunctions::print("count: ", count);
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

    return *(chunks + position.x * chunks_height + position.y);
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

void test_activate_chunk() {
    Grid::new_empty(96, 96);
    auto chunk = chunks + chunks_height + 1;
    auto cell_ptr = cells + 32 + 32 * width;
    *chunk = 0;

    Chunk::activate_neightbors(chunk, 15, 15, cell_ptr + 15 + 15 * width);
    UtilityFunctions::print("activate center: OK");

    Chunk::activate_neightbors(chunk, 0, 0, cell_ptr);
    UtilityFunctions::print("activate top left: OK");

    Chunk::activate_neightbors(chunk, 31, 31, cell_ptr + 31 + 31 * width);
    UtilityFunctions::print("activate bottom right: OK");

    Chunk::activate_neightbors(chunk, 31, 0, cell_ptr + 31);
    UtilityFunctions::print("activate top right: OK");

    Chunk::activate_neightbors(chunk, 0, 31, cell_ptr + 31 * width);
    UtilityFunctions::print("activate bottom left: OK");

    Chunk::activate_neightbors(chunk, 15, 0, cell_ptr + 15);
    UtilityFunctions::print("activate top center: OK");

    Chunk::activate_neightbors(chunk, 15, 31, cell_ptr + 15 + 31 * width);
    UtilityFunctions::print("activate bottom center: OK");
    
    Chunk::activate_neightbors(chunk, 0, 15, cell_ptr + 15 * width);
    UtilityFunctions::print("activate left center: OK");

    Chunk::activate_neightbors(chunk, 31, 15, cell_ptr + 15 + 31 * width);
    UtilityFunctions::print("activate right center: OK");

    Grid::delete_grid();
}

void test_activate_rect() {
    uint64_t *chunk = new uint64_t;
    *chunk = 0;

    Chunk::activate_rect(chunk, 0, 0, 32, 32);
    assert(*chunk == 0xffffffffffffffff);
    UtilityFunctions::print("activate full rect: OK");

    uint32_t rng = 12345789;
    for (int i = 0; i < 10000; i++) {
        *chunk = 0;

        rng = rng * 1103515245 + 12345789;
        int x_offset = rng % 32;
        rng = rng * 1103515245 + 12345789;
        int y_offset = rng % 32;
        rng = rng * 1103515245 + 12345789;
        uint64_t width = rng % (32 - x_offset) + 1;
        rng = rng * 1103515245 + 12345789;
        uint64_t height = rng % (32 - y_offset) + 1;

        Chunk::activate_rect(chunk, x_offset, y_offset, width, height);

        auto rect = Chunk::active_rect(*chunk);
        assert(rect.x_start == x_offset);
        assert(rect.y_start == y_offset);
        assert(rect.x_end == x_offset + width);
        assert(rect.y_end == y_offset + height);
    }
    UtilityFunctions::print("activate random rects: OK");

    delete chunk;
}

} // namespace Test

void Grid::run_tests() {
    UtilityFunctions::print("test_activate_chunk: STARTED");
    Test::test_activate_chunk();
    UtilityFunctions::print("test_activate_chunk: PASSED");

    UtilityFunctions::print("test_activate_rect: STARTED");
    Test::test_activate_rect();
    UtilityFunctions::print("test_activate_rect: PASSED");

    UtilityFunctions::print("All tests passed");
}

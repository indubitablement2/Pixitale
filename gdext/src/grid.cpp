#include <cstdint>
#include <cstring>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image_texture.hpp>

// #define NDEBUG 
#include "assert.h"
#include "godot_cpp/variant/vector2i.hpp"
#include "grid.h"
#include "cell.h"
#include "chunk.h"

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
        CellApi::cell_set_active(cell, true);
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
    
    CellApi::step_updated_bit();
    tick++;

    count = 0;

    for (int column_idx = 1; column_idx < chunks_width - 1; column_idx++) {
        step_column(column_idx);
    }

    UtilityFunctions::print("count: ", count);
}

void Grid::step_column(int column_idx) {
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

void Grid::step_chunk(
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
            auto cell = CellApi(chunk, cell_ptr, local_x, local_y);
            step_cell(cell, rng);
        }
    }
}

void Grid::step_cell(CellApi &cell, uint32_t &rng) {
    if (!cell.is_active() || cell.is_updated()) {
        return;
    }

    bool active = false;
    bool changed = false;

    // Reactions
    // x x x
    // . o x
    // . . .
    CellApi other = cell;
    other.right();
    step_reaction(cell, active, changed, other, rng);
    other.up();
    step_reaction(cell, active, changed, other, rng);
    other.left();
    step_reaction(cell, active, changed, other, rng);
    other.left();
    step_reaction(cell, active, changed, other, rng);

    // TODO: Movement

    if (changed) {
        cell.set_updated();
        cell.set_area_active();
    } else if (active) {
        cell.set_updated();
        cell.set_active(true);
    } else {
        cell.set_active(false);
    }
}

void Grid::step_reaction(CellApi &cell, bool &active, bool &changed, CellApi &other, uint32_t &rng) {
    auto cell_material_idx = cell.material_idx();
    auto other_material_idx = other.material_idx();

    bool swap;
    CellMaterial *mat;
    uint32_t reaction_range_idx;
    if (cell_material_idx > other_material_idx) {
        swap = true;
        mat = CellMaterial::materials + other_material_idx;
        reaction_range_idx = cell_material_idx - other_material_idx;
    } else {
        swap = false;
        mat = CellMaterial::materials + cell_material_idx;
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
                cell.set_material_idx(out1);
                changed = true;
            }

            if (out2 != other_material_idx) {
                other.set_material_idx(out2);
                other.set_area_active();
            }

            return;
        }
    }

    count++;
}

void delete_materials() {
    if (CellMaterial::materials != nullptr) {
        for (int i = 0; i < CellMaterial::materials_len; i++) {
            CellMaterial *mat = CellMaterial::materials + i;
            if (mat->reaction_ranges_len > 0) {
                delete[] mat->reaction_ranges;
                delete[] mat->reactions;
            }
        }

        delete[] CellMaterial::materials;
        CellMaterial::materials = nullptr;
        CellMaterial::materials_len = 0;
    }
}

void Grid::init_materials(int num_materials) {
    delete_materials();

    if (num_materials > 0) {
        CellMaterial::materials = new CellMaterial[num_materials];
        CellMaterial::materials_len = num_materials;
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
    if (CellMaterial::materials == nullptr) {
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

    CellMaterial::materials[idx] = mat;
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

    return CellApi::cell_material_idx(cells[position.x + position.y * width]);
}

void Grid::print_materials() {
    UtilityFunctions::print("num materials: ", CellMaterial::materials_len);

    for (int i = 0; i < CellMaterial::materials_len; i++) {
        CellMaterial &mat = CellMaterial::materials[i];
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
            UtilityFunctions::print("   reaction_start ", reaction_start);
            UtilityFunctions::print("   reaction_end ", reaction_end);
            for (int k = reaction_start; k < reaction_end; k++) {
                CellReaction reaction = *(mat.reactions + k);
                UtilityFunctions::print("       reaction ", k);
                UtilityFunctions::print("       in1 ", i);
                UtilityFunctions::print("       in2 ", i + j);
                UtilityFunctions::print("       probability ", reaction.probability);
                UtilityFunctions::print("       out1 ", reaction.mat_idx_out1);
                UtilityFunctions::print("       out2 ", reaction.mat_idx_out2);
            }
        }
    }
}

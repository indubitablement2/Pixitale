#include <bit>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include "grid.h"

void Grid::_bind_methods() {
    ClassDB::bind_static_method("Grid", D_METHOD("delete_grid"), &Grid::delete_grid);
    ClassDB::bind_static_method("Grid", D_METHOD("new_empty", "width", "height"), &Grid::new_empty);
    ClassDB::bind_static_method("Grid", D_METHOD("get_size"), &Grid::get_size);
    ClassDB::bind_static_method("Grid", D_METHOD("draw_rect", "rect", "on", "at"), &Grid::draw_rect);
    ClassDB::bind_static_method(
        "Grid",
        D_METHOD("set_texture_data", "texture", "rect"),
        &Grid::set_texture_data
    );
    ClassDB::bind_static_method("Grid", D_METHOD("step_manual"), &Grid::step_manual);

    ClassDB::bind_static_method("Grid", D_METHOD("delete_materials"), &Grid::delete_materials);
    ClassDB::bind_static_method("Grid", D_METHOD("init_materials", "num_materials"), &Grid::init_materials);
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

    ClassDB::bind_static_method("Grid", D_METHOD("free_memory"), &Grid::free_memory);
    ClassDB::bind_static_method("Grid", D_METHOD("print_materials"), &Grid::print_materials);

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
    if (_cells != nullptr) {
        delete[] _cells;
        _cells = nullptr;

        delete[] _chunks;
        _chunks = nullptr;
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

    // iterate over all cells
    cell_t i = 0;
    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            _cells[x + y * _width] = i;
            i = (i + 1) % 2;
        }
    }
}

Vector2i Grid::get_size() {
    return Vector2i(_width, _height);
}

void Grid::draw_rect(Rect2i rect, CanvasItem *on, Vector2i at) {
    if (_cells == nullptr) {
        UtilityFunctions::push_warning("Grid is not initialized");
        return;
    }

    rect = rect.intersection(Rect2i(0, 0, _width, _height));

    on->draw_rect(Rect2i(rect.position + at, rect.size), Color(1.0f, 1.0f, 1.0f));
}

void Grid::set_texture_data(Ref<ImageTexture> texture, Rect2i rect) {
    if (_cells == nullptr) {
        UtilityFunctions::push_warning("Grid is not initialized");
        return;
    }

    rect = rect.intersection(Rect2i(0, 0, _width, _height));
    int left = int(rect.position.x);
    int top = int(rect.position.y);
    int right = std::min(int(rect.position.x + rect.size.x), _width);
    int bottom = std::min(int(rect.position.y + rect.size.y), _height);

    int32_t texture_width = texture->get_width();
    int32_t texture_height = texture->get_height();
    int32_t image_width = right - left;
    int32_t image_height = bottom - top;

    bool recreate_texture = false;
    if (texture_width > image_width || texture_height > image_height) {
        if (texture_width - image_width > 256 || texture_height - image_height > 256) {
            recreate_texture = true;
        } else {
            // There will be padding.
            image_width = texture_width;
            image_height = texture_height;
        }
    } else if (texture_width < image_width || texture_height < image_height) {
        recreate_texture = true;
    }
    // TODO: Make clear that only image_width, image_height, left and top are valid.

    // TODO: Reuse this buffer.
    PackedByteArray data = PackedByteArray();
    data.resize(image_width * image_height * sizeof(cell_t));
    
    uint8_t *cells_ptr = reinterpret_cast<uint8_t*>(_cells);
    cells_ptr += left * sizeof(cell_t) + top * _width * sizeof(cell_t);
    uint8_t *data_ptr = data.ptrw();
    
    // Copy each rows.
    for (int i = 0; i < image_height; i++) {
        memcpy(data_ptr, cells_ptr, image_width * sizeof(cell_t));
        cells_ptr += _width * sizeof(cell_t);
        data_ptr += image_width * sizeof(cell_t);
    }

    Ref<Image> image = Image::create_from_data(
        image_width,
        image_height,
        false,
        Image::FORMAT_RGBA8,
        data
    );

    if (recreate_texture) {
        UtilityFunctions::print("Recreating draw texture");
        texture->set_image(image);
    } else {
        texture->update(image);
    }
}

void Grid::step_manual() {
    if (_cells == nullptr) {
        UtilityFunctions::push_warning("Grid is not initialized");
        return;
    }

    _update_bit = (((_update_bit >> CellShifts::CELL_SHIFT_UPDATED) % 3) + 1)
        << CellShifts::CELL_SHIFT_UPDATED;
    _tick++;

    for (int column_idx = 1; column_idx < _chunk_width - 1; column_idx++) {
        step_column(column_idx);
    }
}

cell_t Grid::cell_material_idx(cell_t cell) {
    return cell & CellMasks::CELL_MASK_MATERIAL;
}

void Grid::cell_set_material_idx(cell_t& cell, cell_t material_idx) {
    cell = (cell & ~CellMasks::CELL_MASK_MATERIAL) | material_idx;
}

bool Grid::cell_is_updated(cell_t cell) {
    return (cell & CellMasks::CELL_MASK_UPDATED) == _update_bit;
}

void Grid::cell_set_updated(cell_t& cell) {
    cell = (cell & ~CellMasks::CELL_MASK_UPDATED) | _update_bit;
}

bool Grid::cell_is_active(cell_t cell) {
    return (cell & CellMasks::CELL_MASK_ACTIVE) != 0;
}

// When inactive, set updated bit to 0 (never skip).
void Grid::cell_set_active(cell_t& cell, bool active) {
    if (active) {
        cell |= CellMasks::CELL_MASK_ACTIVE;
    } else {
        cell &= ~CellMasks::CELL_MASK_ACTIVE;
        cell = cell & ~CellMasks::CELL_MASK_UPDATED;
    }
}

// Set cells in a 3x3 area to active (both on cell and chunk).
void Grid::set_area_active(int x, int y, cell_t *center_ptr) {
    chunk_set_active(x - 1, y - 1);
    chunk_set_active(x, y - 1);
    chunk_set_active(x + 1, y - 1);

    chunk_set_active(x - 1, y);
    chunk_set_active(x + 1, y);

    chunk_set_active(x - 1, y + 1);
    chunk_set_active(x, y + 1);
    chunk_set_active(x + 1, y + 1);

    *(center_ptr - 1) |= CellMasks::CELL_MASK_ACTIVE;
    *center_ptr |= CellMasks::CELL_MASK_ACTIVE;
    *(center_ptr + 1) |= CellMasks::CELL_MASK_ACTIVE;
    
    *(center_ptr + _width - 1) |= CellMasks::CELL_MASK_ACTIVE;
    *(center_ptr + _width) |= CellMasks::CELL_MASK_ACTIVE;
    *(center_ptr + _width + 1) |= CellMasks::CELL_MASK_ACTIVE;

    *(center_ptr - _width - 1) |= CellMasks::CELL_MASK_ACTIVE;
    *(center_ptr - _width) |= CellMasks::CELL_MASK_ACTIVE;
    *(center_ptr - _width + 1) |= CellMasks::CELL_MASK_ACTIVE;
}

// Set a single point on chunk to active.
// Does not set cell to active.
void Grid::chunk_set_active(int x, int y) {
    int idx = (y >> 5) + (x >> 5) * _chunk_width;
    chunk_t bitmask = (1 << ((x & 31) + 32)) | (1 << (y & 31));
    chunk_t *chunk = _chunks + idx;
    *chunk |= bitmask;
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

void Grid::step_column(int column_idx) {
    uint64_t rng = (uint64_t)column_idx * _tick;

    chunk_t *chunks_end = _chunks + column_idx * _chunk_height;
    chunk_t *chunks = chunks_end + _chunk_height - 2;
    int num_chunks = _chunk_height - 1;
    
    int origin_x = column_idx * 32;
    int origin_y = _height - 32;

    chunk_t next_chunk = *(chunks - 1);
    *chunks = 0;

    // Iterate over each row from the bottom to the top.
    while (chunks != chunks_end) {
        chunk_t chunk = next_chunk;
        chunks -= 1;
        next_chunk = *chunks;
        *chunks = 0;
        origin_y -= 32;

        if (chunk == 0) {
            continue;
        }

        ChunkActiveRect rect = chunk_active_rect(chunk);
        int x_start = origin_x + rect.column_skip;
        int x_end = x_start + rect.column_count;
        int y_end = origin_y + rect.row_skip - 1;
        int y_start = y_end + rect.row_count;
        
        // Iterate over each cell in the chunk.
        for (int y = y_start; y > y_end; y--) {
            if (chunk_is_row_inactive(chunk, y & 5)) {
                continue;
            }

            for (int x = x_start; x < x_end; x++) {
                step_cell(x, y, rng);
            }
        }
    }
}

void Grid::step_cell(int x, int y, uint64_t &rng) {
    cell_t *cell_ptr = _cells + x + y * _width;
    cell_t cell = *cell_ptr;
    bool active = cell_is_active(cell);

    if (!active) {
        // ? Make sure it is at updated bit == 0.
        return;
    }

    if (cell_is_updated(cell)) {
        return;
    }

    active = false;
    bool changed = false;

    // Reactions
    // x x x
    // . o x
    // . . .
    cell_t cell_mat_idx = cell_material_idx(cell);
    CellMaterial *cell_mat_ptr = _materials + cell_mat_idx;

    cell_t out1;
    cell_t out2;
    cell_t *tl_ptr = cell_ptr - _width - 1;
    cell_t tl = *tl_ptr;
    cell_t tl_mat_idx = cell_material_idx(tl);
    CellMaterial *tl_mat_ptr = _materials + cell_material_idx(tl);
    int tl_result = step_reaction(cell_mat_idx, cell_mat_ptr, tl_mat_idx, tl_mat_ptr, rng, out1, out2);
    if (tl_result == 2) {
        active = true;
        
        if (out2 != tl_mat_idx) {
            cell_set_material_idx(tl, out2);
            *tl_ptr = tl;
            set_area_active(x - 1, y - 1, tl_ptr);
        }

        if (out1 != cell_mat_idx) {
            cell_mat_idx = out1;
            cell_mat_ptr = _materials + cell_mat_idx;
            changed = true;
        }
    } else if (tl_result == 1) {
        active = true;
    }
    
    cell_t *t_ptr = cell_ptr - _width;
    cell_t t = *t_ptr;
    cell_t t_mat_idx = cell_material_idx(t);
    CellMaterial *t_mat_ptr = _materials + cell_material_idx(t);
    int t_result = step_reaction(cell_mat_idx, cell_mat_ptr, t_mat_idx, t_mat_ptr, rng, out1, out2);
    if (t_result == 2) {
        active = true;
        
        if (out2 != t_mat_idx) {
            cell_set_material_idx(t, out2);
            *t_ptr = t;
            set_area_active(x, y - 1, t_ptr);
        }

        if (out1 != cell_mat_idx) {
            cell_mat_idx = out1;
            cell_mat_ptr = _materials + cell_mat_idx;
            changed = true;
        }
    } else if (t_result == 1) {
        active = true;
    }

    cell_t *tr_ptr = cell_ptr - _width + 1;
    cell_t tr = *tr_ptr;
    cell_t tr_mat_idx = cell_material_idx(tr);
    CellMaterial *tr_mat_ptr = _materials + cell_material_idx(tr);
    int tr_result = step_reaction(cell_mat_idx, cell_mat_ptr, tr_mat_idx, tr_mat_ptr, rng, out1, out2);
    if (tr_result == 2) {
        active = true;
        
        if (out2 != tr_mat_idx) {
            cell_set_material_idx(tr, out2);
            *tr_ptr = tr;
            set_area_active(x + 1, y - 1, tr_ptr);
        }

        if (out1 != cell_mat_idx) {
            cell_mat_idx = out1;
            cell_mat_ptr = _materials + cell_mat_idx;
            changed = true;
        }
    } else if (tr_result == 1) {
        active = true;
    }

    cell_t *r_ptr = cell_ptr + 1;
    cell_t r = *r_ptr;
    cell_t r_mat_idx = cell_material_idx(r);
    CellMaterial *r_mat_ptr = _materials + cell_material_idx(r);
    int r_result = step_reaction(cell_mat_idx, cell_mat_ptr, r_mat_idx, r_mat_ptr, rng, out1, out2);
    if (r_result == 2) {
        active = true;
        
        if (out2 != r_mat_idx) {
            cell_set_material_idx(r, out2);
            *r_ptr = r;
            set_area_active(x + 1, y, r_ptr);
        }

        if (out1 != cell_mat_idx) {
            cell_mat_idx = out1;
            cell_mat_ptr = _materials + cell_mat_idx;
            changed = true;
        }
    } else if (r_result == 1) {
        active = true;
    }

    // TODO: Movement

    if (changed) {
        cell_set_updated(cell);
        cell_set_material_idx(cell, cell_mat_idx);
        *cell_ptr = cell;
        set_area_active(x, y, cell_ptr);
    } else if (active) {
        cell_set_updated(cell);
        cell_set_active(cell, true);
        *cell_ptr = cell;
        chunk_set_active(x, y);
    } else {
        cell_set_active(cell, false);
        *cell_ptr = cell;
    }
}

// 0: Can not react
// 1: Could react, but did not
// 2: Reacted
int Grid::step_reaction(
    cell_t mat1_idx,
    CellMaterial *mat1,
    cell_t mat2_idx,
    CellMaterial *mat2,
    uint64_t &rng,
    cell_t &out1,
    cell_t &out2
) {
    bool swap;
    CellMaterial *mat;
    cell_t reaction_range_idx;
    if (mat1_idx > mat2_idx) {
        swap = true;
        mat = mat2;
        reaction_range_idx = mat1_idx - mat2_idx;
    } else {
        swap = false;
        mat = mat1;
        reaction_range_idx = mat2_idx - mat1_idx;
    }

    if (reaction_range_idx >= mat->reaction_ranges_len) {
        return 0;
    }

    uint64_t reaction_range = *(mat->reaction_ranges + reaction_range_idx);
    uint64_t reaction_start = reaction_range & 0xffffffff;
    uint64_t reaction_end = reaction_range >> 32;

    if (reaction_start >= reaction_end) {
        return 0;
    }

    for (uint64_t i = reaction_start; i < reaction_end; i++) {
        CellReaction reaction = *(mat->reactions + i);
        
        rng = rng * 6364136223846792969 + 1442695040888963401;
        if (reaction.probability >= rng) {
            if (swap) {
                out1 = reaction.mat_idx_out2;
                out2 = reaction.mat_idx_out1;
            } else {
                out1 = reaction.mat_idx_out1;
                out2 = reaction.mat_idx_out2;
            }

            return 2;
        }
    }

    return 1;
}

void Grid::delete_materials() {
    if (_materials != nullptr) {
        for (int i = 0; i < _materials_len; i++) {
            CellMaterial *mat = _materials + i;
            if (mat->reaction_ranges_len > 0) {
                delete[] mat->reaction_ranges;
                delete[] mat->reactions;
            }
        }

        delete[] _materials;
        _materials = nullptr;
        _materials_len = 0;
    }
}

void Grid::init_materials(int num_materials) {
    delete_materials();

    if (num_materials == 0) {
        UtilityFunctions::push_error("Number of materials must be greater than 0");
    } else {
        _materials = new CellMaterial[num_materials];
        _materials_len = num_materials;
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
    if (_materials == nullptr) {
        UtilityFunctions::push_error("Materials not initialized");
        return;
    }

    CellMaterial mat;
    mat.cell_movement = cell_movement;
    mat.density = density;
    mat.durability = durability;
    mat.cell_collision = cell_collision;
    mat.friction = friction;

    int num_reaction = 0;
    for (int i = 0; i < reactions.size(); i++) {
        Array r = reactions[i];
        if (!r.is_empty()) {
            mat.reaction_ranges_len = i + 1;
            num_reaction += r.size();
        }
    }
    if (mat.reaction_ranges_len == 0) {
        mat.reaction_ranges = nullptr;
        mat.reactions = nullptr;
    } else {
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

    _materials[idx] = mat;
}

void Grid::free_memory() {
    delete_materials();
    delete_grid();
}

void Grid::print_materials() {
    UtilityFunctions::print("num materials: ", _materials_len);

    for (int i = 0; i < _materials_len; i++) {
        CellMaterial mat = _materials[i];
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
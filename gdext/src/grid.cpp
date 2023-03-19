#include <bit>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include "grid.h"

void Grid::_bind_methods() {
    ClassDB::bind_static_method("Grid", D_METHOD("delete_grid"), &Grid::delete_grid);
    ClassDB::bind_static_method("Grid", D_METHOD("new_empty", "width", "height"), &Grid::new_empty);
    ClassDB::bind_static_method("Grid", D_METHOD("get_size"), &Grid::get_size);
    ClassDB::bind_static_method("Grid", D_METHOD("draw_rect", "rect", "on", "at"), &Grid::draw_rect);
    ClassDB::bind_static_method("Grid", D_METHOD("set_texture_data", "texture", "rect"), &Grid::set_texture_data);

    // ADD_GROUP("Test group", "group_");
	// ADD_SUBGROUP("Test subgroup", "group_subgroup_");

    BIND_CONSTANT(CELL_SIZE);

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

    // iterate over all cells
    cell_t i = 0;
    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            _cells[x + y * _width] = i;
            i = ~i;
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

    // TODO: Reuse this buffer.
    PackedByteArray data = PackedByteArray();
    data.resize(image_width * image_height * sizeof(cell_t));
    
    uint8_t *cells_ptr = reinterpret_cast<uint8_t*>(_cells);
    cells_ptr += left * sizeof(cell_t) + top * _width * sizeof(cell_t);
    uint8_t *data_ptr = data.ptrw();
    
    // Copy each rows.
    for (int y = top; y < bottom; y++) {
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
    int idx = (y >> 5) + (x >> 5) * _chunk_width;
    chunk_t bitmask = (1 << ((x & 31) + 32)) | (1 << (y & 31));
    chunk_t* chunk = _chunks + idx;
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
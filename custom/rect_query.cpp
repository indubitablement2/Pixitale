#include "rect_query.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "grid.h"
#include "preludes.h"

void RectQuery::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rects", "rects", "custom_data"), &RectQuery::set_rects);
	ClassDB::bind_method(D_METHOD("query", "coord"), &RectQuery::query);
	ClassDB::bind_method(D_METHOD("get_rect"), &RectQuery::get_rect);
	ClassDB::bind_method(D_METHOD("get_custom_data"), &RectQuery::get_custom_data);
}

void RectQuery::set_rects(TypedArray<Rect2i> p_rects, Array p_custom_data) {
	last_query_result = nullptr;

	// Find scale based on largest rect.
	scale = Vector2i(0, 0);
	for (i32 i = 0; i < p_rects.size(); i++) {
		Rect2i rect = p_rects[i];
		scale.x = MAX(scale.x, rect.size.x);
		scale.y = MAX(scale.y, rect.size.y);
	}
	scale += Vector2i(1, 1);

	// Place rects into map.
	rects = {};
	for (i32 rect_idx = 0; rect_idx < p_rects.size(); rect_idx++) {
		Rect2i recti = p_rects[rect_idx];
		Rect rect = Rect{
			recti.position.x,
			recti.position.x + recti.size.x,
			recti.position.y,
			recti.position.y + recti.size.y,
			Variant()
		};
		if (rect_idx < p_custom_data.size()) {
			rect.custom_data = p_custom_data[rect_idx];
		}

		Vector2i start = div_floor(recti.position, scale);
		Vector2i end = div_floor(recti.get_end(), scale);

		Vector2i pos[4] = {
			start,
			Vector2i(end.x, start.y),
			Vector2i(start.x, end.y),
			end
		};

		for (i32 pos_idx = 0; pos_idx < 4; pos_idx++) {
			bool duplicate = false;
			for (i32 pos_idx2 = pos_idx - 1; pos_idx2 >= 0; pos_idx2--) {
				if (pos[pos_idx] == pos[pos_idx2]) {
					duplicate = true;
					break;
				}
			}
			if (duplicate) {
				continue;
			}

			rects[Grid::chunk_id(pos[pos_idx])].push_back(rect);
		}
	}

	print_line("scale: ", scale);
	for (auto &pair : rects) {
		print_line(pair.first);
		for (Rect &rect : pair.second) {
			print_line("  ", rect.left, rect.right, rect.top, rect.bottom);
		}
	}
}

bool RectQuery::query(Vector2i coord) {
	Vector2i coord_small = div_floor(coord, scale);
	auto it = rects.find(Grid::chunk_id(coord_small));
	if (it != rects.end()) {
		for (Rect &rect : it->second) {
			if (coord.x >= rect.left && coord.x <= rect.right && coord.y >= rect.top && coord.y <= rect.bottom) {
				last_query_result = &rect;
				return true;
			}
		}
	}

	last_query_result = nullptr;
	return false;
}

Rect2i RectQuery::get_rect() {
	if (last_query_result == nullptr) {
		return Rect2i();
	} else {
		return Rect2i(
				last_query_result->left,
				last_query_result->top,
				last_query_result->right - last_query_result->left,
				last_query_result->bottom - last_query_result->top);
	}
}

Variant RectQuery::get_custom_data() {
	if (last_query_result == nullptr) {
		return Rect2i();
	} else {
		return last_query_result->custom_data;
	}
}
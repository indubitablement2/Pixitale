#ifndef RECT_QUERY_H
#define RECT_QUERY_H

#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"
#include "preludes.h"
#include <unordered_map>
#include <vector>

class RectQuery : public RefCounted {
	GDCLASS(RectQuery, RefCounted);

protected:
	static void _bind_methods();

private:
	struct Rect {
		i32 left;
		i32 right;
		i32 top;
		i32 bottom;
		Variant custom_data;
	};

	Rect *last_query_result = nullptr;
	Vector2i scale;
	std::unordered_map<u64, std::vector<Rect>> rects;

public:
	void set_rects(TypedArray<Rect2i> p_rects, Array p_custom_data);

	bool query(Vector2i coord);
	Rect2i get_rect();
	Variant get_custom_data();
};

#endif
#include "cell_material.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "preludes.h"
#include "rng.hpp"
#include <vector>

u32 CellMaterial::get_value_at(const Vector2i coord, Rng &rng) {
	u32 value = 0;

	if (values_width != 0) {
		u32 x = coord.x % values_width;
		u32 y = coord.y % values_height;
		value = (u32)values[x + y * values_width];
	}

	if (max_value_noise != 0) {
		value = rng.gen_range_u32(0, max_value_noise);
		value = MIN(value, 15u);
	}

	return value;
}

u32 CellMaterial::get_hue_at(const Vector2i coord, Rng &rng) {
	// todo
	return 0;
}

CellMaterial::CellMaterial(Object *obj) {
	collision = CellCollision(i32(obj->get("collision_type", nullptr)));
	// collision = VariantCaster<CellCollision>::cast(obj->get("collision_type", nullptr));
}
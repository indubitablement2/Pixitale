#ifndef GENERATION_H
#define GENERATION_H

#include "core/math/vector2i.h"
#include "preludes.h"

#include "modules/noise/fastnoise_lite.h"
#include "scene/resources/curve.h"

class Generation : public Object {
	GDCLASS(Generation, Object);

protected:
	static void _bind_methods();

public:
	static void set_cell(Vector2i position, u32 cell_material_idx);
	static void set_cell_rect(Rect2i rect, u32 cell_material_idx);

	static void cavern_pass(
			Ref<Curve> horizontal_gradient,
			Ref<Curve> vertical_gradient,
			Ref<FastNoiseLite> cavern,
			f32 cavern_x_scale,
			f32 cavern_threshold);
};

#endif
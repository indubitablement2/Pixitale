#ifndef GENERATION_H
#define GENERATION_H

#include "core/typedefs.h"
#include "modules/noise/fastnoise_lite.h"
#include "scene/resources/curve.h"
#include <cstdint>

class Generation : public Object {
	GDCLASS(Generation, Object);

protected:
	static void _bind_methods();

public:
	static void cavern_pass(
			Ref<Curve> horizontal_gradient,
			Ref<Curve> vertical_gradient,
			Ref<FastNoiseLite> cavern,
			float cavern_x_scale,
			uint32_t surface_top);
};

#endif
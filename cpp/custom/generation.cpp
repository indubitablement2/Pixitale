#include "generation.h"

#include "core/typedefs.h"
#include "grid.h"
#include <cstdint>

void Generation::_bind_methods() {
	ClassDB::bind_static_method(
			"Generation",
			D_METHOD(
					"cavern_pass",
					"horizontal_gradient",
					"vertical_gradient",
					"cavern",
					"cavern_x_scale"),
			&Generation::cavern_pass);
}

void Generation::cavern_pass(
		Ref<Curve> horizontal_gradient,
		Ref<Curve> vertical_gradient,
		Ref<FastNoiseLite> cavern,
		float cavern_x_scale,
		uint32_t surface_top) {
	ERR_FAIL_COND_MSG(Grid::cells == nullptr, "Grid is not initialized");

	surface_top = MAX(0u, surface_top);

	horizontal_gradient->bake();
	vertical_gradient->bake();

	for (uint32_t y = surface_top; y < Grid::height; y++) {
		float vg = vertical_gradient->sample_baked((float)y / (float)Grid::height);

		for (uint32_t x = 0; x < Grid::width; x++) {
			float hg = horizontal_gradient->sample_baked((float)x / (float)Grid::width);

			float value = cavern->get_noise_2d((float)x * cavern_x_scale, (float)y) * vg * hg;

			if (value > 0.5) {
				Grid::cells[x + y * Grid::width] = 0;
			}
		}
	}
}
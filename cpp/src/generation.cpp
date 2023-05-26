#include "generation.h"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "grid.h"
#include <cmath>

using namespace godot;

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
		int surface_top) {
	horizontal_gradient->bake();
	vertical_gradient->bake();

	// const auto BAKE = 128;

	// float vg_bake[BAKE + 1];
	// for (int i = 0; i < BAKE + 1; i++) {
	// 	vg_bake[i] = vertical_gradient->sample_baked((double)i / (double)BAKE);
	// }

	// float hg_bake[BAKE + 1];
	// for (int i = 0; i < BAKE + 1; i++) {
	// 	hg_bake[i] = horizontal_gradient->sample_baked((double)i / (double)BAKE);
	// }

	for (int y = surface_top; y < Grid::height; y++) {
		float vg = vertical_gradient->sample_baked((double)y / (double)Grid::height);
		// float pf = ((float)y * (float)BAKE) / (float)Grid::height;
		// int pi = (int)pf;
		// float t = pf - (float)pi;
		// auto p1 = vg_bake[pi];
		// auto p2 = vg_bake[pi + 1];
		// auto vg = Math::lerp(p1, p2, t);

		for (int x = 0; x < Grid::width; x++) {
			float hg = horizontal_gradient->sample_baked((double)x / (double)Grid::width);
			// pf = ((float)x * (float)BAKE) / (float)Grid::width;
			// pi = (int)pf;
			// t = pf - (float)pi;
			// p1 = hg_bake[pi];
			// p2 = hg_bake[pi + 1];
			// auto hg = Math::lerp(p1, p2, t);

			// auto value = cavern->get_noise_2d((float)x * cavern_x_scale, (float)y) * vg * hg;

			if (vg * hg > 0.5) {
				Grid::cells[x + y * Grid::width] = 0;
			}
		}
	}
}
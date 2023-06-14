#include "generation.h"

#include "grid.h"

void Generation::_bind_methods() {
	ClassDB::bind_static_method(
			"Generation",
			D_METHOD(
					"surface_pass",
					"rock",
					"dirt",
					"surface_top",
					"surface_bot"),
			&Generation::surface_pass);

	ClassDB::bind_static_method(
			"Generation",
			D_METHOD(
					"cavern_pass",
					"horizontal_gradient",
					"vertical_gradient",
					"cavern",
					"cavern_x_scale",
					"surface_top",
					"cavern_threshold"),
			&Generation::cavern_pass);
}

void Generation::surface_pass(
		u32 rock,
		u32 dirt,
		i32 surface_top,
		i32 surface_bot

) {
	// Fill under surface layer with rock.
	for (i32 y = surface_bot; y < Grid::height; y++) {
		for (i32 x = 0; x < Grid::width; x++) {
			Grid::cells[x + y * Grid::width] = rock;
		}
	}
}

void Generation::cavern_pass(
		Ref<Curve> horizontal_gradient,
		Ref<Curve> vertical_gradient,
		Ref<FastNoiseLite> cavern,
		f32 cavern_x_scale,
		i32 surface_top,
		f32 cavern_threshold) {
	ERR_FAIL_COND_MSG(Grid::cells == nullptr, "Grid is not initialized");

	surface_top = MAX(0, surface_top);

	horizontal_gradient->bake();
	vertical_gradient->bake();

	for (i32 y = surface_top; y < Grid::height; y++) {
		f32 vg = vertical_gradient->sample_baked((f32)y / (f32)Grid::height);

		for (i32 x = 0; x < Grid::width; x++) {
			f32 hg = horizontal_gradient->sample_baked((f32)x / (f32)Grid::width);

			f32 value = cavern->get_noise_2d((f32)x * cavern_x_scale, (f32)y) * vg * hg;

			if (value > cavern_threshold) {
				Grid::cells[x + y * Grid::width] = 0;
			}
		}
	}
}
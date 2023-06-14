#include "generation.h"

#include "grid.h"

void Generation::_bind_methods() {
	ClassDB::bind_static_method(
			"Generation",
			D_METHOD(
					"set_cell",
					"position",
					"cell_material_idx"),
			&Generation::set_cell);
	ClassDB::bind_static_method(
			"Generation",
			D_METHOD(
					"set_cell_rect",
					"rect",
					"cell_material_idx"),
			&Generation::set_cell_rect);

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

void Generation::set_cell(Vector2i position, u32 cell_material_idx) {
	if (position.x < 0 || position.x >= Grid::width || position.y < 0 || position.y >= Grid::height) {
		return;
	}

	*(Grid::cells + position.y * Grid::width + position.x) = cell_material_idx;
}

void Generation::set_cell_rect(Rect2i rect, u32 cell_material_idx) {
	rect = rect.intersection(Rect2i(0, 0, Grid::width, Grid::height));

	for (i32 y = rect.position.y; y < rect.get_end().y; y++) {
		for (i32 x = rect.position.x; x < rect.get_end().x; x++) {
			*(Grid::cells + y * Grid::width + x) = cell_material_idx;
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
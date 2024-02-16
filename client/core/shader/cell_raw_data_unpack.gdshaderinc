const uint MASK_MATERIAL_IDX = 16383u;

const uint MASK_ACTIVE = 65536u;

const uint SHIFT_COLOR_RED = 17u;
const uint SHIFT_COLOR_GREEN = 22u;
const uint SHIFT_COLOR_BLUE = 27u;
const uint MASK_COLOR_CHANNEL = 31u;

const float COLOR_CHANNEL_RANGE = 31.0;

// Modulo which handle negative numbers instead of returning the remainder.
// Eg. wrap(-1, 10) -> 9 whereas -1 % 10 -> -1.
// Taken from prelude.hpp `mod_neg`
int wrap(int numerator, int denominator) {
	int m = numerator % denominator;
	return m >= 0 ? m : m + denominator;
}

ivec2 wrapv(ivec2 numerator, ivec2 denominator) {
	return ivec2(
		wrap(numerator.x, denominator.x),
		wrap(numerator.y, denominator.y)
	);
}

uint get_cell_data(sampler2D data_tex, ivec2 local_coords) {
	return floatBitsToUint(texelFetch(data_tex, local_coords, 0).x);
}

uint get_cell_material_idx(uint data) {
	return data & MASK_MATERIAL_IDX;
}

bool is_cell_active(uint data) {
	return (data & MASK_ACTIVE) != 0u;
}

vec3 get_cell_color_modulate(uint data) {
	return vec3(
		float((data >> SHIFT_COLOR_RED) & MASK_COLOR_CHANNEL) / COLOR_CHANNEL_RANGE,
		float((data >> SHIFT_COLOR_GREEN) & MASK_COLOR_CHANNEL) / COLOR_CHANNEL_RANGE,
		float(data >> SHIFT_COLOR_BLUE) / COLOR_CHANNEL_RANGE
	);
}

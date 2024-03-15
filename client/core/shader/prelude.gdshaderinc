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

uint get_cell_data(sampler2D data_tex, ivec2 local_coord) {
	return floatBitsToUint(texelFetch(data_tex, local_coord, 0).x);
}

//uint get_cell_data_neirest(sampler2D data_tex, vec2 uv) {
	//return floatBitsToUint(texture(data_tex, uv).x);
//}

int get_cell_material_idx(uint data) {
	return int(data & 4095u);
}

bool is_cell_active(uint data) {
	return (data &  65536u) != 0u;
}

// Already inverted.
// [0, 1]
float get_cell_darken(uint data) {
	return float(((data >> 20u) & 63u) ^ 63u) / 63.0;
}

int get_cell_color(uint data) {
	return int(data >> 26u);
}

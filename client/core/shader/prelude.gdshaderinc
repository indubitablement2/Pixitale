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

float _get_channel(uint v) {
	float f = float(v);
	f /= 31.0f;
	if (f > 0.5f) {
		f = -f + 0.5f;
	}
	return f;
}

// [-0.5..0.5]
vec3 get_cell_color_modulate(uint data) {
	return vec3(
		_get_channel((data >> 17u) & 31u),
		_get_channel((data >> 22u) & 31u),
		_get_channel(data >> 27u)
	);
}

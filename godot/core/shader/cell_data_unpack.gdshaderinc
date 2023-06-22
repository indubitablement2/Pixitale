const uint MASK_MATERIAL_IDX = 4095u;

const uint MASK_COLOR = 15u;
const uint SHIFT_HUE = 24u;
const uint SHIFT_VALUE = 28u;

const int CELL_DATA_BASE_COLOR = 0;
const int CELL_DATA_GLOW = 1;
const int CELL_DATA_LIGHT_BLOCK = 2;
global uniform sampler2D cell_materials_data;

global uniform sampler2D cell_modulate_palette;

uint get_cell_data(sampler2D data_tex, ivec2 local_coords) {
	return floatBitsToUint(texelFetch(data_tex, local_coords, 0).x);
}

vec4 get_cell_modulate(uint data) {
	uint hue_idx = (data >> SHIFT_HUE) & MASK_COLOR;
	uint value_idx = (data >> SHIFT_VALUE);
	
	return texelFetch(
		cell_modulate_palette,
		ivec2(int(hue_idx), int(value_idx)),
		0
	);
}

vec4 get_cell_base_color(uint data, ivec2 global_coords) {
	uint material_idx = data & MASK_MATERIAL_IDX;
	
	return texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), CELL_DATA_BASE_COLOR),
		0
	);
}

vec4 get_cell_glow(uint data) {
	uint material_idx = data & MASK_MATERIAL_IDX;
	
	return texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), CELL_DATA_GLOW),
		0
	);
}

vec4 get_cell_light_block(uint data) {
	uint material_idx = data & MASK_MATERIAL_IDX;
	
	return texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), CELL_DATA_LIGHT_BLOCK),
		0
	);
}

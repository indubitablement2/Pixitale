const uint MASK_MATERIAL_IDX = 16383u;

const uint MASK_ACTIVE = 65536u;

const uint SHIFT_COLOR_RED = 17u;
const uint SHIFT_COLOR_GREEN = 22u;
const uint SHIFT_COLOR_BLUE = 27u;
const uint MASK_COLOR_CHANNEL = 31u;

const float COLOR_CHANNEL_RANGE = 31.0;

const uint CELL_DATA_BASE_COLOR_START = 0u;
const uint CELL_DATA_BASE_COLOR_SIZE = 1u;
const uint CELL_DATA_GLOW = 2u;
const uint CELL_DATA_LIGHT_MODULATE = 3u;
const uint CELL_DATA_SIZE = 4u;
const uint CELL_MATERIALS_DATA_WIDTH = 512u;
uniform sampler2D cell_materials_data;

uint get_cell_data(sampler2D data_tex, ivec2 local_coords) {
	return floatBitsToUint(texelFetch(data_tex, local_coords, 0).x);
}

vec3 get_cell_color_modulate(uint data) {
	return vec3(
		float((data >> SHIFT_COLOR_RED) & MASK_COLOR_CHANNEL) / COLOR_CHANNEL_RANGE,
		float((data >> SHIFT_COLOR_GREEN) & MASK_COLOR_CHANNEL) / COLOR_CHANNEL_RANGE,
		float(data >> SHIFT_COLOR_BLUE) / COLOR_CHANNEL_RANGE
	);
}

ivec2 _get_cell_base_color_start(uint material_idx) {
	uint idx = material_idx * CELL_DATA_SIZE + CELL_DATA_BASE_COLOR_START;
	
	ivec2 coord = ivec2(
		int(idx % CELL_MATERIALS_DATA_WIDTH),
		int(idx / CELL_MATERIALS_DATA_WIDTH)
	);
	
	vec4 data = texelFetch(
		cell_materials_data,
		coord,
		0
	);
	
	return ivec2(
		int(data.x * 255.0 * 255.0 + data.y * 255.0),
		int(data.z * 255.0 * 255.0 + data.w * 255.0)
	);
}

ivec2 _get_cell_base_color_size(uint material_idx) {
	uint idx = material_idx * CELL_DATA_SIZE + CELL_DATA_BASE_COLOR_SIZE;
	
	ivec2 coord = ivec2(
		int(idx % CELL_MATERIALS_DATA_WIDTH),
		int(idx / CELL_MATERIALS_DATA_WIDTH)
	);
	
	vec4 data = texelFetch(
		cell_materials_data,
		coord,
		0
	);
	
	return ivec2(
		int(data.x * 255.0 * 255.0 + data.y * 255.0),
		int(data.z * 255.0 * 255.0 + data.w * 255.0)
	);
}

vec4 get_cell_base_color(uint data, ivec2 global_coords, sampler2D base_color_tex) {
	ivec2 start = _get_cell_base_color_start(data & MASK_MATERIAL_IDX);
	ivec2 size = _get_cell_base_color_size(data & MASK_MATERIAL_IDX);
	
	ivec2 coord = start + ivec2(
		global_coords.x % size.x,
		global_coords.y % size.y
	);
	
	return texelFetch(
		base_color_tex,
		coord,
		0
	);
}

vec4 get_cell_glow(uint material_idx) {
	uint idx = material_idx * CELL_DATA_SIZE + CELL_DATA_GLOW;
	
	ivec2 coord = ivec2(
		int(idx % CELL_MATERIALS_DATA_WIDTH),
		int(idx / CELL_MATERIALS_DATA_WIDTH)
	);
	
	return texelFetch(
		cell_materials_data,
		coord,
		0
	);
}

vec4 get_cell_light_modulate(uint material_idx) {
	uint idx = material_idx * CELL_DATA_SIZE + CELL_DATA_LIGHT_MODULATE;
	
	ivec2 coord = ivec2(
		int(idx % CELL_MATERIALS_DATA_WIDTH),
		int(idx / CELL_MATERIALS_DATA_WIDTH)
	);
	
	return texelFetch(
		cell_materials_data,
		coord,
		0
	);
}

bool is_cell_active(uint data) {
	return (data & MASK_ACTIVE) != 0u;
}

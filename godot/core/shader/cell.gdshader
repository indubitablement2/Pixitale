shader_type canvas_item;

const uint MASK_MATERIAL_IDX = 4095u;

const uint MASK_COLOR = 15u;
const uint SHIFT_HUE = 24u;
const uint SHIFT_VALUE = 28u;

global uniform sampler2D cell_materials_data;
global uniform sampler2D cell_modulate_palette;

void fragment() {
	ivec2 local_coords = ivec2((UV * vec2(textureSize(TEXTURE, 0))));
	uint data = floatBitsToUint(texelFetch(TEXTURE, local_coords, 0).x);
//	uint data = floatBitsToUint(texture(TEXTURE, UV).x);
	
	// Unpack cell data
	uint material_idx = data & MASK_MATERIAL_IDX;
	uint hue_idx = (data >> SHIFT_HUE) & MASK_COLOR;
	uint value_idx = (data >> SHIFT_VALUE);
	
	// Get base color
	vec4 col = texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), 0),
		0
	);
	
	// Apply modulate
	col *= texelFetch(
		cell_modulate_palette,
		ivec2(int(hue_idx), int(value_idx)),
		0
	);
	
//	// Apply hue
//	col *= texelFetch(
//		hue_palette,
//		ivec2(int(hue_idx), 0),
//		0
//	);
//
//	// Apply value
//	col *= texelFetch(
//		value_palette,
//		ivec2(int(value_idx), 0),
//		0
//	);
	
//	ivec2 global_coords = offset + local_coords;
	
//	// Test texture
//	ivec2 tex_local_coords = global_coords % textureSize(tex, 0);
//	col *= texelFetch(
//		tex,
//		tex_local_coords,
//		0
//	);
	
	COLOR = col;
}

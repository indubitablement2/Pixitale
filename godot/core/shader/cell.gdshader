shader_type canvas_item;

const uint MASK_MATERIAL_IDX = 4095u;

const uint MASK_COLOR = 15u;
const uint SHIFT_HUE = 24u;
const uint SHIFT_VALUE = 28u;

uniform sampler2D cell_materials_data;
uniform sampler2D hue_palette;
uniform sampler2D value_palette;

void fragment() {
	// Unpack cell data
	ivec2 coords = ivec2((UV * vec2(textureSize(TEXTURE, 0))));
	uint data = floatBitsToUint(texelFetch(TEXTURE, coords, 0).x);
//	uint data = floatBitsToUint(texture(TEXTURE, UV).x);
	
	uint material_idx = data & MASK_MATERIAL_IDX;
	uint hue_idx = (data >> SHIFT_HUE) & MASK_COLOR;
	uint value_idx = (data >> SHIFT_VALUE) & MASK_COLOR;
	
	// Get base color
	vec4 col = texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), 0),
		0
	);
	
	// Apply hue
	col *= texelFetch(
		hue_palette,
		ivec2(int(hue_idx), 0),
		0
	);
	
	// Apply value
	col *= texelFetch(
		value_palette,
		ivec2(int(value_idx), 0),
		0
	);
	
	COLOR = col;
}

shader_type canvas_item;

const uint MATERIAL_IDX_MASK = 4095u;

uniform sampler2D cell_materials_data;

void fragment() {
	
	ivec2 coords = ivec2((UV * vec2(textureSize(TEXTURE, 0))));
	uint data = floatBitsToUint(texelFetch(TEXTURE, coords, 0).x);

	uint material_idx = data & MATERIAL_IDX_MASK;
	
	vec4 base_color = texelFetch(
		cell_materials_data,
		ivec2(int(material_idx), 0),
		0
	);
	
	float r = float((material_idx * 3u + 1u) % 11u) / 11.0;
	float g = float((material_idx * 5u + 3u) % 13u) / 13.0;
	float b = float((material_idx * 7u + 5u) % 17u) / 17.0;
	
	vec4 col = vec4(r, g, b, 1.0);
	
	COLOR = base_color;
}

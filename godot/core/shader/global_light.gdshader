shader_type canvas_item;
render_mode blend_add;

#include "cell_data_unpack.gdshaderinc"

uniform vec3 light : source_color = vec3(0.5, 0.5, 0.5);

//uniform ivec2 global_origin;

uniform int global_light_local_end = 0;

void fragment() {
	ivec2 local_coords = ivec2(VERTEX);
//	ivec2 global_coords = ivec2(VERTEX) + global_origin;
	
	uint data = get_cell_data(TEXTURE, local_coords);
	
//	COLOR = get_cell_base_color(data, ivec2(0));
	
	// Global light.
	vec3 col = light;
	
	vec4 light_block = get_cell_light_block(data);
	col *= light_block.rgb;
//	col *= 1.0 - light_block.a;
	// It is already inverted.
	col *= light_block.a;
	
	col *= float(local_coords.y < global_light_local_end);
	
	// Cell glow.
	col += get_cell_glow(data).rgb;
	
	COLOR = vec4(col, 1.0);
	
//	// Global raycast light (sun).
//	vec3 sun = light;
//	vec3 ray_col = light;
//	vec2 slope = vec2(-2.0, -2.0);
//	float mul;
//	for (int i = 1; i < 30; i += 1) {
//		color_pos += slope;
//		mul = float(i) * 0.75;
//
////		cell_color = texture(TEXTURE, color_pos / vec2(textureSize(TEXTURE, 0)));
//		cell_color = texelFetch(TEXTURE, ivec2(color_pos), 0);
//		sun *= (1.0 - (cell_color.a * cell_color.a) / mul);
//		ray_col += cell_color.rgb;
//	}
//	ray_col /= 30.0;
//	col *= 0.75;
//	col += sun * ray_col * 0.25;
}

shader_type canvas_item;
render_mode blend_add;

uniform vec3 light : source_color;

uniform vec2 global_origin;
uniform vec2 color_offset;
uniform float no_light_start;
uniform float no_light_end;

void fragment() {
	//	vec2 global_pos = VERTEX + global_origin;
	
	// Global light.
	vec2 color_pos = VERTEX + color_offset;
	ivec2 color_posi = ivec2(VERTEX + color_offset);
	vec4 cell_color = texelFetch(TEXTURE, color_posi, 0);
	
	vec3 col = light * cell_color.rgb;
	col *= 1.0 - cell_color.a * cell_color.a;
	
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
	
	COLOR = vec4(col, 1.0);
}

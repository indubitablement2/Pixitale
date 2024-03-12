shader_type canvas_item;
render_mode blend_add;

#include "prelude.gdshaderinc"

uniform ivec2 origin;

uniform sampler2D glow;
uniform sampler2D light_modulate;

uniform sampler2D background;

/// Light emitted when the background isn't opaque.
uniform vec3 background_light_color : source_color = vec3(0.5, 0.5, 0.5);

/// Raycast light emitted when the background isn't opaque.
uniform vec3 raycast_light_color : source_color = vec3(0.5, 0.5, 0.5);
uniform bool raycast_light_enabled = false;
uniform vec2 raycast_light_step = vec2(-2.0, -3.464);
uniform int raycast_light_num_step = 8;

void fragment() {
	ivec2 local_coords = ivec2(UV.xy * vec2(textureSize(TEXTURE, 0)));
	ivec2 global_coord = local_coords + origin;
	
	uint data = get_cell_data(TEXTURE, local_coords);
	int idx = int(get_cell_material_idx(data));
	
	vec3 g = texelFetch(glow, ivec2(idx, 0), 0).rgb;
	
//	COLOR = get_cell_base_color(data, ivec2(0));
	
	// Global light.
	//vec3 col = light;
	vec3 col = g;
	
	//vec4 light_block = get_cell_light_block(data);
	//col *= light_block.rgb;
////	col *= 1.0 - light_block.a;
	//// It is already inverted.
	//col *= light_block.a;
	//
	//col *= float(local_coords.y < global_light_local_end);
	//
	//// Cell glow.
	//col += get_cell_glow(data).rgb;
	
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

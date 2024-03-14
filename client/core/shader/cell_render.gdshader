shader_type canvas_item;
//render_mode blend_disabled;

#include "prelude.gdshaderinc"

const float COLOR_MODULATE_STRENGTH = 0.3;

global uniform ivec2 cell_buffer_origin;
global uniform vec2 cell_buffer_size;

uniform sampler2D base_color_atlas;
uniform sampler2D base_color_atlas_rect;

uniform bool highlight_active = false;

void fragment() {
	ivec2 local_coord = ivec2(UV.xy * cell_buffer_size);
	ivec2 global_coord = local_coord + cell_buffer_origin;
	
	uint data = get_cell_data(TEXTURE, local_coord);
	int idx = get_cell_material_idx(data);
	
	vec4 base_color_rect = texelFetch(base_color_atlas_rect, ivec2(idx, 0), 0);
	ivec2 base_color_start = ivec2(base_color_rect.xy);
	ivec2 base_color_size = ivec2(base_color_rect.zw);
	
	vec4 color = texelFetch(
		base_color_atlas,
		wrapv(global_coord, base_color_size) + base_color_start,
		0);
	
	vec3 color_modulate = get_cell_color_modulate(data);
	color.rgb += color_modulate * COLOR_MODULATE_STRENGTH;
	
	float same_neighbors = 1.0;
	same_neighbors += 0.18 * float(
		get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(0, -2))) != idx
		|| get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(2, 0))) != idx
		//|| get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(1, -1))) != idx
	);
	same_neighbors -= 0.22 * float(
		get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(-2, 0))) != idx
		|| get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(0, 2))) != idx
		//|| get_cell_material_idx(get_cell_data(TEXTURE, local_coord + ivec2(-1, 1))) != idx
	);
	color.rgb *= same_neighbors;
	
	if (highlight_active) {
		if (is_cell_active(data)) {
			color += vec4(0.1, 0.1, 0.1, 0.3);
			color.a = min(color.a, 1.0);
		}
	}
	
	COLOR = color;
}

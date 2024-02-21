shader_type canvas_item;
//render_mode blend_disabled;

#include "cell_raw_data_unpack.gdshaderinc"

const float COLOR_MODULATE_STRENGTH = 0.5;

uniform ivec2 origin;

uniform sampler2D base_color_atlas;
uniform sampler2D base_color_atlas_rect;

uniform bool highlight_active = false;

void fragment() {
	ivec2 local_coords = ivec2(UV.xy * vec2(textureSize(TEXTURE, 0)));
	ivec2 global_coord = local_coords + origin;
	
	uint data = get_cell_data(TEXTURE, local_coords);
	int idx = int(get_cell_material_idx(data));
	
	vec4 base_color_rect = texelFetch(base_color_atlas_rect, ivec2(idx, 0), 0);
	ivec2 base_color_start = ivec2(base_color_rect.xy);
	ivec2 base_color_size = ivec2(base_color_rect.zw);
	
	vec4 color = texelFetch(
		base_color_atlas,
		wrapv(global_coord, base_color_size) + base_color_start,
		0);
	
	//vec3 color_modulate = get_cell_color_modulate(data);
	//
	//color.rgb = color.rgb * (1.0 - COLOR_MODULATE_STRENGTH)
		//+ (color.rgb * color_modulate * COLOR_MODULATE_STRENGTH);
	
	if (highlight_active) {
		if (is_cell_active(data)) {
			color += vec4(0.1, 0.1, 0.1, 0.3);
		}
	}
	
	//if (idx == 0) {
		//color = vec4(1.0, 0.0, 0.0, 0.5);
	//} else {
		//color = vec4(0.0, 1.0, 0.0, 1.0);
	//}
	//color.rg = VERTEX / vec2(128.0);
	//color.rg = UV;
	//color.rg = vec2(wrapv(global_coord, ivec2(64))) / 64.0;
	//color.ba = vec2(0.0, 1.0);
	
	//if (global_coord.x < 0) {
		//color.r = 1.0;
	//} else {
		//color.r = 0.0;
	//}
	//if (global_coord.y < 0) {
		//color.g = 1.0;
	//} else {
		//color.g = 0.0;
	//}
	
	COLOR = color;
}

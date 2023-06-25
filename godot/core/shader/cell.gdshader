shader_type canvas_item;
//render_mode blend_disabled;

#define SHOW_ACTIVE

#include "cell_data_unpack.gdshaderinc"

void fragment() {
	ivec2 local_coords = ivec2((UV * vec2(textureSize(TEXTURE, 0))));
	uint data = get_cell_data(TEXTURE, local_coords);
	
	vec4 col = get_cell_base_color(data, ivec2(VERTEX));
	col *= get_cell_modulate(data);
	
	#ifdef SHOW_ACTIVE
	if (is_cell_active(data)) {
		col += vec4(0.0, 0.0, 1.0, 1.0);
	}
	#endif
	
	COLOR = col;
}

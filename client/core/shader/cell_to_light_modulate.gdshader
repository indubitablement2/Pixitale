shader_type canvas_item;

#include "prelude.gdshaderinc"

global uniform vec2 cell_buffer_size;

uniform sampler2D light_modulate;

void fragment() {
	ivec2 local_coord = ivec2(UV.xy * cell_buffer_size);
	
	uint data = get_cell_data(TEXTURE, local_coord);
	int idx = get_cell_material_idx(data);
	
	vec4 col = texelFetch(light_modulate, ivec2(idx, 0), 0);
	
	COLOR = col;
}


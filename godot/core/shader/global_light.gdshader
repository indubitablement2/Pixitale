shader_type canvas_item;
render_mode blend_add;

uniform vec3 light : source_color;

uniform vec2 global_origin;
uniform vec2 color_offset;
uniform float no_light_start;
uniform float no_light_end;

void fragment() {
	vec2 color_pos = VERTEX + color_offset;
	vec4 cell_color = texelFetch(TEXTURE, ivec2(color_pos), 0);
	
	vec3 col = light * cell_color.rgb;
	col *= 1.0 - cell_color.a;
	
//	col = vec4(1.0) - cell_color;
//	col = vec4(1.0 - cell_color.a);
//	col = cell_color;
	
//	vec2 global_pos = global_origin + VERTEX;
//	if (global_pos.y > no_light_start && global_pos.y < no_light_end) {
//		col = vec4(0.0);
//	}
	
	COLOR = vec4(col, 1.0);
}

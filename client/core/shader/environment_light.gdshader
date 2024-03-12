shader_type canvas_item;
// Use this to "clear" the screen.
// Otherwise, blend_add should be used for most light.
render_mode blend_disabled;

#include "prelude.gdshaderinc"

global uniform vec2 cell_buffer_size;

uniform vec3 light_color : source_color = vec3(0.5, 0.5, 0.5);

uniform sampler2D background;

void fragment() {
	ivec2 local_coord = ivec2(UV.xy * cell_buffer_size);
	
	vec4 col = vec4(light_color, 1.0);
	
	vec4 modulate =  texelFetch(TEXTURE, local_coord, 0);
	modulate *= modulate; // ^2
	modulate *= modulate; // ^4
	modulate *= modulate; // ^8
	col *= modulate;
	
	modulate = texelFetch(background, local_coord, 0);
	modulate *= modulate;
	modulate *= modulate;
	modulate *= modulate;
	col *= modulate;
	
	COLOR = col;
}


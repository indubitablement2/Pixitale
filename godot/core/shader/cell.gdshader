shader_type canvas_item;

void fragment() {
	vec4 data = texture(TEXTURE, UV);
	vec4 col = vec4(0.0, 0.0, 0.0, 1.0);
	if (data.x != 0.0) {
		col = vec4(1.0);
	}
	
	COLOR = col;
}

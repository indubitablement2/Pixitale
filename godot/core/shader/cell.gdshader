shader_type canvas_item;

void fragment() {
	vec4 data = texture(TEXTURE, UV);
	
	uint mat_idx = uint(data.x * 255.0);
	float r = float((mat_idx * 3u) % 11u) / 11.0;
	float g = float((mat_idx * 5u) % 13u) / 13.0;
	float b = float((mat_idx * 7u) % 17u) / 17.0;
	
	vec4 col = vec4(r, g, b, 1.0);
	
	COLOR = col;
}

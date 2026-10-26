shader_type canvas_item;

void vertex() {
	// Called for every vertex the material is visible on.
}

void fragment() {
	vec4 tex_col = texture(TEXTURE, UV + -sin(TIME) * sin(TIME) * cos(TIME) * 0.1);
	COLOR.rgb = tex_col.rgb;
}

//void light() {
//	// Called for every pixel for every light affecting the CanvasItem.
//	// Uncomment to replace the default light processing function with this one.
//}

@tool
extends TextureRect

@export var _create := false : set = create

func create(value: bool) -> void:
	if !value:
		return
	
	var img := Image.create(64, 1, false, Image.FORMAT_RGB8)
	
	# Greyscale
	for i in 10:
		var v := float((i + 5) % 10) / 9.0
		img.set_pixel(i, 0, Color.from_ok_hsl(0.0, 0.0, v))
	
	# Pure color
	img.set_pixel(10, 0, Color(1.0, 0.0, 0.0))
	img.set_pixel(11, 0, Color(1.0, 1.0, 0.0))
	img.set_pixel(12, 0, Color(0.0, 1.0, 0.0))
	img.set_pixel(13, 0, Color(0.0, 1.0, 1.0))
	img.set_pixel(14, 0, Color(0.0, 0.0, 1.0))
	img.set_pixel(15, 0, Color(1.0, 0.0, 1.0))
	
	# Oklab
	for i in 48:
		var h := float(i) / 47.0
		var l := float((i % 8) + 1) / 10.0
		img.set_pixel(i + 16, 0, Color.from_ok_hsl(h, 1.0, l))
	
	texture = ImageTexture.create_from_image(img)
	
	#print("[")
	#for i in 64:
		#print("\tColor", img.get_pixel(i, 0), ",")
	#print("]")

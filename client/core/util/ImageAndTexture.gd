extends Resource
class_name ImageAndTexture

## Encapsulate an Image and an ImageTexture which display this Image.

## If modifying the Image's pixels (set_pixel, fill),
## you have to call emit_changed for the texture to be updated.
@export var img : Image : set = set_img
func set_img(value: Image) -> void:
	if img:
		img.changed.disconnect(_on_changed)
	img = value
	if img:
		img.changed.connect(_on_changed)
	_on_changed()
## self.get_img_mut().fill() instead of
## self.img.fill(); self.emit_changed()
func get_img_mut() -> Image:
	_on_changed()
	return img

## Automatically updated at the end of the frame when its Image changes.
@export var tex : ImageTexture
#func set_tex(value: ImageTexture) -> void:
	#if tex:
		#tex.changed.disconnect(_on_changed)
	#tex = value
	#if tex:
		#tex.changed.connect(_on_changed)
	#emit_changed()

@export var print_on_resize := false

var _will_update_texture := false

func _init() -> void:
	changed.connect(_on_changed)

func _on_changed() -> void:
	if !_will_update_texture:
		_update_texture.call_deferred()
		_will_update_texture = true

func _update_texture() -> void:
	_will_update_texture = false
	
	if !img || !tex:
		return
	if img.is_empty():
		return
	
	if img.get_size() == Vector2i(tex.get_size()):
		tex.update(img)
	else:
		tex.set_image(img)
		if print_on_resize:
			print_debug(resource_name, " size changed: ", img.get_size())
	

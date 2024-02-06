#ifndef IMAGE_PACKER_H
#define IMAGE_PACKER_H

#include "core/io/image.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/variant/callable.h"
#include "core/variant/typed_array.h"

class ImagePacker : public Object {
	GDCLASS(ImagePacker, Object);

protected:
	static void _bind_methods();

	static Ref<Image> pack(
			TypedArray<Image> images,
			Image::Format format,
			StringName meta_name);
};

#endif
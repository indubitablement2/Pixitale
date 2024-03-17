#include "register_types.h"
#include "core/object/class_db.h"
#include "grid.h"
#include "grid_body.h"
#include "grid_iter.h"
#include "image_packer.h"
#include "tests.h"

void initialize_custom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_abstract_class<PixitaleTests>();
	ClassDB::register_abstract_class<Grid>();
	ClassDB::register_abstract_class<ImagePacker>();

	ClassDB::register_class<GridChunkIter>();
	ClassDB::register_class<GridRectIter>();
	ClassDB::register_class<GridLineIter>();

	ClassDB::register_class<GridBody>();
}

void uninitialize_custom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

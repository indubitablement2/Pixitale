#include "register_types.h"
#include "cell_material.h"
#include "cell_reaction.h"
#include "core/object/class_db.h"
#include "generation_pass.h"
#include "grid.h"
#include "grid_iter.h"
#include "tests.h"

void initialize_custom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_abstract_class<PixitaleTests>();
	ClassDB::register_abstract_class<Grid>();

	ClassDB::register_class<CellMaterial>();
	ClassDB::register_class<CellReaction>();

	ClassDB::register_class<GridIter>();
	ClassDB::register_class<GenerationPass>();

	// ClassDB::register_class<GridCharacterBody>();
	// ClassDB::register_class<GridBiomeScanner>();
}

void uninitialize_custom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

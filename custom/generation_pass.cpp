#include "generation_pass.h"

void GenerationPass::_bind_methods() {
	// _generate = StringName("generate");

	GDVIRTUAL_BIND(_generate, "api");
}

void GenerationPass::_clear_generation_passes() {
	generation_passes.clear();
}

void GenerationPass::_add_generation_pass(GenerationPass *pass) {
	generation_passes.push_back(pass);
}

void GenerationPass::generate(GridChunkIter *api) {
	GDVIRTUAL_CALL(_generate, api);
}

#include "generation_pass.h"

void GenerationPass::_bind_methods() {
	ClassDB::bind_static_method(
			"GenerationPass",
			D_METHOD("_clear_generation_passes"),
			&GenerationPass::_clear_generation_passes);
	ClassDB::bind_static_method(
			"GenerationPass",
			D_METHOD("_add_generation_pass", "pass"),
			&GenerationPass::_add_generation_pass);

	GDVIRTUAL_BIND(_generate, "api");
}

void GenerationPass::_clear_generation_passes() {
	generation_passes.clear();
}

void GenerationPass::_add_generation_pass(GenerationPass *pass) {
	generation_passes.push_back(pass);
}

void GenerationPass::generate_all(GridChunkIter *api) {
	for (GenerationPass *pass : generation_passes) {
		pass->generate(api);
	}
}

void GenerationPass::generate(GridChunkIter *api) {
	GDVIRTUAL_CALL(_generate, api);
}

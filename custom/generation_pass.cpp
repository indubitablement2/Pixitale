#include "generation_pass.h"

void GenerationPass::_bind_methods() {
	// _generate = StringName("generate");

	GDVIRTUAL_BIND(_generate, "api");
}

void GenerationPass::generate(GridIter *api) {
	GDVIRTUAL_CALL(_generate, api);
}

#include "generation_pass.h"

// #include "grid.h"

void GenerationPass::_bind_methods() {
	GDVIRTUAL_BIND(_generate, "api");
}

void GenerationPass::generate(GenerationApi *api) {
	GDVIRTUAL_CALL(_generate, api);
}

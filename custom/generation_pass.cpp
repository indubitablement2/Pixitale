#include "generation_pass.h"

void GenerationPass::_bind_methods() {
	GDVIRTUAL_BIND(_generate, "iter");
}

void GenerationPass::generate(GridChunkIter *iter) {
	GDVIRTUAL_CALL(_generate, iter);
}
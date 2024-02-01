#ifndef GENERATION_PASS_H
#define GENERATION_PASS_H

#include "core/variant/array.h"
#include "grid_iter.h"
#include "scene/main/node.h"
#include <vector>

class GenerationPass : public Node {
	GDCLASS(GenerationPass, Node);

protected:
	static void _bind_methods();

public:
	inline static std::vector<GenerationPass *> generation_passes = {};

	static void _clear_generation_passes();
	static void _add_generation_pass(GenerationPass *pass);

	void generate(GridChunkIter *api);

	GDVIRTUAL1(_generate, GridChunkIter *);
};

#endif
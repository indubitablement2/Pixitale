#ifndef GENERATION_PASS_H
#define GENERATION_PASS_H

#include "grid_iter.h"
#include "scene/main/node.h"

class GenerationPass : public Node {
	GDCLASS(GenerationPass, Node);

protected:
	static void _bind_methods();

public:
	void generate(GridChunkIter *iter);

	GDVIRTUAL1(_generate, GridChunkIter *);
};

#endif
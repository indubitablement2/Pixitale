#ifndef GENERATION_PASS_H
#define GENERATION_PASS_H

#include "generation_api.h"
#include "scene/main/node.h"

class GenerationPass : public Node {
	GDCLASS(GenerationPass, Node);

protected:
	static void _bind_methods();

public:
	void generate(GenerationApi *api);

	GDVIRTUAL1(_generate, GenerationApi *);
};

#endif
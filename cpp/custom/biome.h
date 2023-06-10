#ifndef BIOME_H
#define BIOME_H

#include "preludes.h"

#include "core/object/class_db.h"
#include "scene/2d/node_2d.h"

struct Biome {
	u32 min_cell_biome_count;
	f32 min_depth;
	f32 min_distance_from_center;
};

class GridBiomeScanner : public Node2D {
	GDCLASS(GridBiomeScanner, Node2D);

protected:
	static void _bind_methods();

private:
	inline static std::vector<Biome> biomes = {};
	// Reuse this vector for scanning.
	inline static std::vector<u32> cell_biome_counts = {};

	u32 current_biome_idx = 0;

public:
	bool scan();
	u32 get_current_biome();

	static void set_biomes(std::vector<Biome> new_biomes);

	GridBiomeScanner(){};
	~GridBiomeScanner(){};
};

#endif
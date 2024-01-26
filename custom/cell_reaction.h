#ifndef CELL_REACTION_H
#define CELL_REACTION_H

#include "core/io/resource.h"
#include "core/string/string_name.h"
#include "preludes.h"
#include "rng.hpp"
#include "vector"
#include <unordered_map>

const u32 CELL_REACTION_PROBABILITY_RANGE = 1 << 16;

struct CellReactionPacked {
	// Chance for reaction to happen out of CELL_REACTION_PROBABILITY_RANGE.
	u16 probability;

	// If eq in1 does not change material.
	u16 mat_idx_out1;
	// If eq in2 does not change material.
	u16 mat_idx_out2;

	// todo: optional gdscript function
	// Use highest bit to indicate if callback is present.
	u16 reaction_idx_and_has_callback;

	inline bool try_react(Rng &rng) {
		return rng.gen_probability_u32(probability, CELL_REACTION_PROBABILITY_RANGE);
	}

	inline bool has_callback() {
		return reaction_idx_and_has_callback & 0x8000;
	}

	inline u16 get_reaction_idx() {
		return reaction_idx_and_has_callback & 0x7FFF;
	}
};

class CellReaction : public Resource {
	GDCLASS(CellReaction, Resource);

protected:
	static void _bind_methods();

public:
	inline static std::vector<Ref<CellReaction>> reactions = {};
	// Key is lower material_idx | higher material_idx << 16.
	inline static std::unordered_map<u32, std::vector<CellReactionPacked>> reactions_map = {};

	static void add_reaction(Ref<CellReaction> value);

	// Set pointers to nullptr if no reaction between m1 and m2.
	static void reactions_between(
			CellReactionPacked *&start,
			CellReactionPacked *&end,
			u32 m1,
			u32 m2,
			bool &swap);

public:
	StringName in1_tag;
	StringName in2_tag;
	void set_in1_tag(StringName value);
	StringName get_in1_tag() const;
	void set_in2_tag(StringName value);
	StringName get_in2_tag() const;

	StringName out1_id;
	StringName out2_id;
	void set_out1_id(StringName value);
	StringName get_out1_id() const;
	void set_out2_id(StringName value);
	StringName get_out2_id() const;

	f64 probability = 1.0;
	void set_probability(f64 value);
	f64 get_probability() const;

	u16 compute_probability();

	i32 reaction_idx = -1;

	bool enabled = false;
	void set_enable(bool value);
	bool get_enable() const;

	void repack();
};

#endif
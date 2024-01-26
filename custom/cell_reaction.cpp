#include "cell_reaction.h"
#include "cell_material.h"
#include "core/error/error_macros.h"
#include "core/object/object.h"
#include "core/string/string_name.h"
#include "core/typedefs.h"
#include "preludes.h"

void tag_to_material_idx(StringName &tag, u32 *&start, u32 *&end) {
	if (auto it = CellMaterial::material_tags.find(tag.data_unique_pointer()); it != CellMaterial::material_tags.end()) {
		start = it->second.data();
		end = start + it->second.size();
	} else {
		start = nullptr;
		end = nullptr;
		ERR_FAIL_MSG("Unknown cell material tag: " + tag);
	}
}

u32 reations_key(const u32 m1, const u32 m2, bool &swap) {
	if (m1 <= m2) {
		swap = false;
		return m1 | (m2 << 16);
	} else {
		swap = true;
		return m2 | (m1 << 16);
	}
}

void CellReaction::_bind_methods() {
	ClassDB::bind_static_method(
			"CellReaction",
			D_METHOD("add_reaction", "reaction"),
			&CellReaction::add_reaction);

	ClassDB::bind_method(D_METHOD("set_in1_tag", "value"), &CellReaction::set_in1_tag);
	ClassDB::bind_method(D_METHOD("get_in1_tag"), &CellReaction::get_in1_tag);
	ClassDB::bind_method(D_METHOD("set_in2_tag", "value"), &CellReaction::set_in2_tag);
	ClassDB::bind_method(D_METHOD("get_in2_tag"), &CellReaction::get_in2_tag);

	ClassDB::bind_method(D_METHOD("set_out1_id", "value"), &CellReaction::set_out1_id);
	ClassDB::bind_method(D_METHOD("get_out1_id"), &CellReaction::get_out1_id);
	ClassDB::bind_method(D_METHOD("set_out2_id", "value"), &CellReaction::set_out2_id);
	ClassDB::bind_method(D_METHOD("get_out2_id"), &CellReaction::get_out2_id);

	ClassDB::bind_method(D_METHOD("set_probability", "value"), &CellReaction::set_probability);
	ClassDB::bind_method(D_METHOD("get_probability"), &CellReaction::get_probability);

	ClassDB::bind_method(D_METHOD("set_enable", "value"), &CellReaction::set_enable);
	ClassDB::bind_method(D_METHOD("get_enable"), &CellReaction::get_enable);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "in1_tag"), "set_in1_tag", "get_in1_tag");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "in2_tag"), "set_in2_tag", "get_in2_tag");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "out1_id"), "set_out1_id", "get_out1_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "out2_id"), "set_out2_id", "get_out2_id");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "probability", PROPERTY_HINT_RANGE, "0.000015259,1.0,0.000015259"), "set_probability", "get_probability");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable"), "set_enable", "get_enable");
}

void CellReaction::add_reaction(Ref<CellReaction> value) {
	value->reaction_idx = reactions.size();
	reactions.push_back(value);

	if (value->enabled) {
		value->enabled = false;
		value->set_enable(true);
	}
}

void CellReaction::reactions_between(
		CellReactionPacked *&start,
		CellReactionPacked *&end,
		u32 m1,
		u32 m2,
		bool &swap) {
	u32 reactions_key = reations_key(
			m1,
			m2,
			swap);

	if (auto it = reactions_map.find(reactions_key); it != reactions_map.end()) {
		auto v = &it->second;
		start = v->data();
		end = start + v->size();
	} else {
		start = nullptr;
		end = nullptr;
	}
}

void CellReaction::set_in1_tag(StringName value) {
	in1_tag = value;
	repack();
}

StringName CellReaction::get_in1_tag() const {
	return in1_tag;
}

void CellReaction::set_in2_tag(StringName value) {
	in2_tag = value;
	repack();
}

StringName CellReaction::get_in2_tag() const {
	return in2_tag;
}

void CellReaction::set_out1_id(StringName value) {
	out1_id = value;
	repack();
}

StringName CellReaction::get_out1_id() const {
	return out1_id;
}

void CellReaction::set_out2_id(StringName value) {
	out2_id = value;
	repack();
}

StringName CellReaction::get_out2_id() const {
	return out2_id;
}

void CellReaction::set_probability(f64 value) {
	probability = value;
	repack();
}

f64 CellReaction::get_probability() const {
	return probability;
}

u16 CellReaction::compute_probability() {
	return u16(CLAMP(
			probability * (f64)CELL_REACTION_PROBABILITY_RANGE,
			0.0,
			(f64)(MAX_U16)));
}

void CellReaction::set_enable(bool value) {
	if (enabled == value) {
		return;
	}
	enabled = value;

	if (reaction_idx < 0) {
		return;
	}

	u32 *in1_ptr;
	u32 *in1_end;
	tag_to_material_idx(in1_tag, in1_ptr, in1_end);
	u32 *in2_ptr;
	u32 *in2_end;
	tag_to_material_idx(in2_tag, in2_ptr, in2_end);

	// Runtime changes.
	if (enabled) {
		u32 out1;
		if (auto it = CellMaterial::material_ids.find(out1_id.data_unique_pointer()); it != CellMaterial::material_ids.end()) {
			out1 = it->second;
		} else {
			enabled = false;
			ERR_FAIL_MSG("Unknown cell material id: " + out1_id);
		}
		u32 out2;
		if (auto it = CellMaterial::material_ids.find(out2_id.data_unique_pointer()); it != CellMaterial::material_ids.end()) {
			out2 = it->second;
		} else {
			enabled = false;
			ERR_FAIL_MSG("Unknown cell material id: " + out2_id);
		}

		u16 computed_probability = compute_probability();

		while (in1_ptr != in1_end) {
			while (in2_ptr != in1_end) {
				u32 in1 = *in1_ptr;
				u32 in2 = *in2_ptr;

				bool swap;
				u32 reaction_key = reations_key(in1, in2, swap);

				CellReactionPacked packed_reaction = {};
				packed_reaction.probability = computed_probability;
				packed_reaction.reaction_idx_and_has_callback = u16(reaction_idx);
				if (swap) {
					packed_reaction.mat_idx_out1 = out2;
					packed_reaction.mat_idx_out2 = out1;
				} else {
					packed_reaction.mat_idx_out1 = out1;
					packed_reaction.mat_idx_out2 = out2;
				}

				if (auto it = reactions_map.find(reaction_key); it != reactions_map.end()) {
					it->second.push_back(packed_reaction);
				} else {
					reactions_map[reaction_key] = { packed_reaction };
				}

				in2_ptr += 1;
			}
			in1_ptr += 1;
		}
	} else {
		while (in1_ptr != in1_end) {
			while (in2_ptr != in1_end) {
				u32 in1 = *in1_ptr;
				u32 in2 = *in2_ptr;

				bool swap;
				u32 reaction_key = reations_key(in1, in2, swap);

				if (auto it = reactions_map.find(reaction_key); it != reactions_map.end()) {
					for (auto itt = it->second.begin(); itt != it->second.end();) {
						if (itt->get_reaction_idx() == reaction_idx) {
							it->second.erase(itt);
						} else {
							++itt;
						}
					}

					if (it->second.empty()) {
						reactions_map.erase(it);
					}
				}

				in2_ptr += 1;
			}
			in1_ptr += 1;
		}
	}
}

bool CellReaction::get_enable() const {
	return enabled;
}

void CellReaction::repack() {
	if (enabled) {
		set_enable(false);
		set_enable(true);
	}
}
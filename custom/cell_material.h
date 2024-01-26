#ifndef CELL_MATERIAL_H
#define CELL_MATERIAL_H

#include "core/io/image.h"
#include "core/io/resource.h"
#include "core/math/vector2i.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/variant/typed_array.h"
#include "preludes.h"
#include "rng.hpp"
#include "vector"
#include <unordered_map>
#include <vector>

const u32 DUPLICATE_ON_VERTICAL_MOVEMENT_PROBABILITY_RANGE = 1 << 20;

enum CellCollision {
	COLLISION_NONE,
	COLLISION_SOLID,
	COLLISION_PLATFORM,
	COLLISION_LIQUID,
};

class CellMaterial : public Resource {
	GDCLASS(CellMaterial, Resource);

protected:
	static void _bind_methods();

public:
	static std::vector<Ref<CellMaterial>> materials;
	// Material id to material idx.
	static std::unordered_map<const void *, u32> material_ids;
	// Tag to material idx.
	static std::unordered_map<const void *, std::vector<u32>> material_tags;

	static void add_material(Ref<CellMaterial> value);

	// Meant for gdscript.
	// Return 0 if not found.
	static u32 find_material_idx(StringName material_id);
	// Return nullptr if not found.
	static Ref<CellMaterial> find_material(StringName material_id);
	// Return nullptr if not found.
	static Ref<CellMaterial> get_material(u32 material_idx);

public:
	StringName material_id;
	void set_material_id(StringName value);
	StringName get_material_id();

	TypedArray<StringName> tags = { StringName("_all") };
	void set_tags(TypedArray<StringName> value);
	TypedArray<StringName> get_tags();

	u32 material_idx = 0;
	u32 get_material_idx();

	CellCollision collision = CellCollision::COLLISION_NONE;
	void set_collision(CellCollision value);
	CellCollision get_collision();

	Ref<Image> values_image;
	std::vector<u8> values = {};
	u32 values_height = 0;
	u32 values_width = 0;
	void set_values_image(Ref<Image> value);
	Ref<Image> get_values_image();
	u32 get_value_at(const Vector2i coord, Rng &rng);

	// f32 durability = 0.0f;
	// f32 friction;
	// u32 biome_contribution = 0;

	// If 0, then no noise.
	u32 max_value_noise = 0;

	// Can swap position with less dense cell.
	i32 density = 0;

	// How much velocity gained per step.
	// Negative value moves upwars.
	f32 vertical_acceleration = 1.0f;
	// How many vertical movement per step when at max orthogonal velocity.
	// Fractional part is handled using probability.
	f32 vertical_velocity_max = 4.0f;
	// TODO: wind effect when moving vertically.

	// How much orthogonal velocity gained/lost per step when moving horizontally.
	// Should be set to 1 for fluid and some negative value otherwise.
	f32 horizontal_acceleration;
	// How many horizontal movement per step when at max orthogonal velocity.
	f32 horizontal_velocity_max;
	// Duplicate this cell on vertical movement when moving atop inactive cells.
	// This is for top layer of fluid to eventually fill up and become inactive
	// instead of moving back and forth forever.
	u32 duplicate_on_vertical_movement_probability;

	// When true, cell will fall when not supported by other cells.
	bool can_fall = false;
	bool can_color = false;

	u32 get_hue_at(const Vector2i coord, Rng &rng);

	void print();
};

VARIANT_ENUM_CAST(CellCollision);

#endif // CELL_MATERIAL_H
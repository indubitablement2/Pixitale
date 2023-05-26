#ifndef GENERATION_H
#define GENERATION_H

#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>

using namespace godot;

class Generation : public Object {
	GDCLASS(Generation, Object);

protected:
	static void _bind_methods();

public:
	static void cavern_pass(
			Ref<Curve> horizontal_gradient,
			Ref<Curve> vertical_gradient,
			Ref<FastNoiseLite> cavern,
			float cavern_x_scale,
			int surface_top);
};

#endif
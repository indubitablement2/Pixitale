#ifndef RNG_HPP
#define RNG_HPP

#include "preludes.h"
#include <cstring>

struct Rng {
	u64 state;

	Rng() :
			state(0) {}

	Rng(u64 seed) :
			state(seed) {
		state += 0x9e3779b97f4a7c15uLL;
		state ^= state >> 30;
		state *= 0xbf58476d1ce4e5b9uLL;
		state ^= state >> 27;
		state *= 0x94d049bb133111ebuLL;
		state ^= (state >> 31);
	}

	inline void mix(u64 seed) {
		state = Rng(state ^ seed).state;
	}

	inline void next() {
		state = state * 0x27BB2EE687B0B0FDuLL + 0xB504F32DuLL;
	}

	inline u32 gen_u32() {
		next();
		return state >> 32;
	}

	inline u32 gen_range_u32(u32 min, u32 max) {
		TEST_ASSERT(min < max, "min must be less than max");
		TEST_ASSERT(max > 0, "max must be greater than 0");

		return (gen_u32() % (max - min)) + min;
	}

	inline i32 gen_range_i32(i32 min, i32 max) {
		TEST_ASSERT(min < max, "min must be less than max");
		TEST_ASSERT(max > 0, "max must be greater than 0");

		return i32(gen_u32() % u32(max - min)) + min;
	}

	inline bool gen_bool() {
		next();
		return state & 1uLL << 37;
	}

	// Return 1 or -1. 50% chance of either.
	inline i32 gen_sign() {
		return gen_bool() ? 1 : -1;
	}

	// Return a float in the range [0..1[
	inline f32 gen_f32() {
		next();
		u32 u = u32(state >> 41) | 0x3F800000u;
		f32 f;
		std::memcpy(&f, &u, 4);
		return f - 1.0f;
	}

	inline f32 gen_range_f32(f32 min, f32 max) {
		TEST_ASSERT(min < max, "min must be less than max");
		return gen_f32() * (max - min) + min;
	}

	// probability >= 1 always return true.
	// probability <= 0 always return false.
	inline bool gen_probability_f32(f32 probability) {
		return gen_f32() < probability;
	}

	// probability >= range always return true.
	// probability = 0 always return false.
	// Might be faster if range is a constant power of 2.
	inline bool gen_probability_u32(u32 probability, u32 range) {
		return gen_range_u32(0, range) < probability;
	}
};

#endif
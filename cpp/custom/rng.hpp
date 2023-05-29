#ifndef RNG_HPP
#define RNG_HPP

#include "preludes.h"

namespace Rng {

inline void next(u64 &rng) {
	rng = rng * 2862933555777941757uLL + 3037000493uLL;
}

inline u32 gen_32bit(u64 &rng) {
	next(rng);
	return rng >> 32;
}

inline u32 gen_range_32bit(u64 &rng, u32 min, u32 max) {
	assert(min < max);
	assert(max > 0);

	return (gen_32bit(rng) % (max - min)) + min;
}

inline bool gen_bool(u64 &rng) {
	return gen_32bit(rng) & 1;
}

} // namespace Rng

#endif
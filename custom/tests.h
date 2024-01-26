#ifndef PIXITALE_TESTS_H
#define PIXITALE_TESTS_H

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "modules/noise/fastnoise_lite.h"
#include "preludes.h"

class PixitaleTests : public Object {
	GDCLASS(PixitaleTests, Object);

protected:
	static void _bind_methods();

public:
	static void run_tests();
	static bool assert_enabled();

	static f32 test_perf(Ref<FastNoiseLite> noise, i32 size);
};

#endif
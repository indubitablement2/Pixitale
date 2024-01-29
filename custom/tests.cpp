#include "tests.h"
#include "core/math/vector2i.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "preludes.h"
#include "rng.hpp"

void test_iter2d() {
	Iter2D iter = Iter2D(Vector2i(-10, -10), Vector2i(10, 10));
	for (i32 y = -10; y != 10; y++) {
		for (i32 x = -10; x != 10; x++) {
			TEST_ASSERT(iter.next(), "iter2d");
			TEST_ASSERT(iter.coord == Vector2i(x, y), "iter2d");
		}
	}
	TEST_ASSERT(!iter.next(), "iter2d");

	iter = Iter2D(Vector2i(10, 10), Vector2i(-10, -10));
	for (i32 y = 10; y != -10; y--) {
		for (i32 x = 10; x != -10; x--) {
			TEST_ASSERT(iter.next(), "iter2d");
			TEST_ASSERT(iter.coord == Vector2i(x, y), "iter2d");
		}
	}
	TEST_ASSERT(!iter.next(), "iter2d");

	iter = Iter2D(Vector2i(1, 1), Vector2i(1, 8));
	TEST_ASSERT(!iter.next(), "empty x");
	iter = Iter2D(Vector2i(1, 1), Vector2i(8, 1));
	TEST_ASSERT(!iter.next(), "empty y");
}

void test_int_coord() {
	ChunkLocalCoord c = ChunkLocalCoord(Vector2i(-1, -1));
	TEST_ASSERT(c.chunk_coord == Vector2i(-1, -1), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(31, 31), "int coord");

	c = ChunkLocalCoord(Vector2i(-31, -31));
	TEST_ASSERT(c.chunk_coord == Vector2i(-1, -1), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(1, 1), "int coord");

	c = ChunkLocalCoord(Vector2i(-32, -32));
	TEST_ASSERT(c.chunk_coord == Vector2i(-1, -1), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(0, 0), "int coord");

	c = ChunkLocalCoord(Vector2i(-33, -33));
	TEST_ASSERT(c.chunk_coord == Vector2i(-2, -2), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(31, 31), "int coord");

	c = ChunkLocalCoord(Vector2i(-64, -64));
	TEST_ASSERT(c.chunk_coord == Vector2i(-2, -2), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(0, 0), "int coord");

	c = ChunkLocalCoord(Vector2i(0, 0));
	TEST_ASSERT(c.chunk_coord == Vector2i(0, 0), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(0, 0), "int coord");

	c = ChunkLocalCoord(Vector2i(31, 31));
	TEST_ASSERT(c.chunk_coord == Vector2i(0, 0), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(31, 31), "int coord");

	c = ChunkLocalCoord(Vector2i(32, 32));
	TEST_ASSERT(c.chunk_coord == Vector2i(1, 1), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(0, 0), "int coord");

	c = ChunkLocalCoord(Vector2i(33, 33));
	TEST_ASSERT(c.chunk_coord == Vector2i(1, 1), "int coord");
	TEST_ASSERT(c.local_coord == Vector2i(1, 1), "int coord");
}

void test_iter_chunk() {
	const i32 START_CHUNK = -2;
	const i32 END_CHUNK = 2;
	const i32 WIDTH_CHUNK = END_CHUNK - START_CHUNK;
	const i32 SIZE_CHUNK = WIDTH_CHUNK * WIDTH_CHUNK;
	const i32 START = -33;
	const i32 END = 33;
	const i32 WIDTH = END - START;
	const i32 SIZE = WIDTH * WIDTH;
	bool visited_cell[SIZE] = { false };
	bool visited_chunk[SIZE_CHUNK] = { false };

	IterChunk c = IterChunk(Rect2i(START, START, WIDTH, WIDTH));
	// print_line("from: ", c._start.chunk_coord, " - ", c._start.local_coord);
	// print_line("to: ", c._end.chunk_coord, " - ", c._end.local_coord);
	while (c.next()) {
		// print_line(c.chunk_coord, ": ", c.local_coord_start, " - ", c.local_coord_end);

		TEST_ASSERT(c.chunk_coord.x >= START_CHUNK, "chunk coord oob");
		TEST_ASSERT(c.chunk_coord.x < END_CHUNK, "chunk coord oob");
		TEST_ASSERT(c.chunk_coord.y >= START_CHUNK, "chunk coord oob");
		TEST_ASSERT(c.chunk_coord.y < END_CHUNK, "chunk coord oob");

		i32 i = (c.chunk_coord.x - START_CHUNK) + (c.chunk_coord.y - START_CHUNK) * WIDTH_CHUNK;
		TEST_ASSERT(i < SIZE_CHUNK, "chunk coord oob");

		TEST_ASSERT(!visited_chunk[i], "chunk coord visited twice");
		visited_chunk[i] = true;

		Iter2D iter = Iter2D(c.local_coord_start + c.chunk_coord * 32, c.local_coord_end + c.chunk_coord * 32);
		while (iter.next()) {
			TEST_ASSERT(iter.coord.x >= START, "local coord oob");
			TEST_ASSERT(iter.coord.x < END, "local coord oob");
			TEST_ASSERT(iter.coord.y >= START, "local coord oob");
			TEST_ASSERT(iter.coord.y < END, "local coord oob");

			i32 ii = (iter.coord.x - START) + (iter.coord.y - START) * WIDTH;
			TEST_ASSERT(ii < SIZE, "local coord oob");

			TEST_ASSERT(!visited_cell[ii], "local coord visited twice");
			visited_cell[ii] = true;
		}
	}

	for (i32 i = 0; i < SIZE_CHUNK; i++) {
		TEST_ASSERT(visited_chunk[i], "chunk coord not visited");
	}
	for (i32 i = 0; i < SIZE; i++) {
		TEST_ASSERT(visited_cell[i], "cell not visited");
	}

	c = IterChunk(Vector2i(-5, -5));
	TEST_ASSERT(c.next(), "iter chunk");
	Iter2D cell_iter = c.local_iter();
	bool v2[32 * 32] = { false };
	while (cell_iter.next()) {
		TEST_ASSERT(v2[cell_iter.coord.x + cell_iter.coord.y * 32] == false, "already visited");
		v2[cell_iter.coord.x + cell_iter.coord.y * 32] = true;
	}
	for (i32 i = 0; i < 32 * 32; i++) {
		TEST_ASSERT(v2[i], "not visited");
	}
}

void test_rng_bias() {
	u32 num_tests = 100000;
	u32 num_true = 0;
	f64 float_bias = 0.0f;

	Rng rng = Rng(123);

	for (u32 i = 0; i < num_tests; i++) {
		if (rng.gen_bool()) {
			num_true++;
		}
		float_bias += (f64)rng.gen_f32() / (f64)num_tests;
	}

	f64 true_bias = (f64)num_true / (f64)num_tests;
	// print_line("rng true bias ", num_true, "/", num_tests);
	// print_line("rng float bias ", float_bias);

	TEST_ASSERT(true_bias > 0.45 && true_bias < 0.55, "rng bool bias");
	TEST_ASSERT(true_bias != 0.5, "rng true bias is 0.5");
	TEST_ASSERT(float_bias > 0.45 && float_bias < 0.55, "rng float bias");
	TEST_ASSERT(float_bias != 0.5, "rng float bias is 0.5");
}

void PixitaleTests::_bind_methods() {
	ClassDB::bind_static_method(
			"PixitaleTests",
			D_METHOD("run_tests"),
			&PixitaleTests::run_tests);

	ClassDB::bind_static_method(
			"PixitaleTests",
			D_METHOD("assert_enabled"),
			&PixitaleTests::assert_enabled);

	ClassDB::bind_static_method(
			"PixitaleTests",
			D_METHOD("test_perf", "noise", "size"),
			&PixitaleTests::test_perf);
}

void PixitaleTests::run_tests() {
	if (!assert_enabled()) {
		return;
	}

	test_iter2d();
	test_int_coord();
	test_iter_chunk();
	test_rng_bias();
}

bool PixitaleTests::assert_enabled() {
#ifdef TOOLS_ENABLED
	return true;
#else
	return false;
#endif
}

f32 PixitaleTests::test_perf(Ref<FastNoiseLite> noise, i32 size) {
	i64 start = Time::get_singleton()->get_ticks_msec();
	f32 sum = 0.0;
	for (i32 y = 0; y < size; y++) {
		for (i32 x = 0; x < size; x++) {
			sum += noise->get_noise_2d(x, y);
		}
	}
	i64 end = Time::get_singleton()->get_ticks_msec();

	print_line("elapsed: ", end - start, "ms");

	return sum;
}
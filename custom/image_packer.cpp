#include "image_packer.h"
#include "core/math/color.h"
#include "core/math/rect2i.h"
#include "core/math/vector2i.h"
#include <algorithm>
#include <array>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// rect_structs.h
namespace rectpack2D {
using total_area_type = int;

struct rect_wh {
	rect_wh() :
			w(0), h(0) {}
	rect_wh(const int w, const int h) :
			w(w), h(h) {}

	int w;
	int h;

	auto &flip() {
		std::swap(w, h);
		return *this;
	}

	int max_side() const {
		return h > w ? h : w;
	}

	int min_side() const {
		return h < w ? h : w;
	}

	int area() const { return w * h; }
	int perimeter() const { return 2 * w + 2 * h; }

	float pathological_mult() const {
		return float(max_side()) / min_side() * area();
	}

	template <class R>
	void expand_with(const R &r) {
		w = std::max(w, r.x + r.w);
		h = std::max(h, r.y + r.h);
	}
};

struct rect_xywh {
	int x;
	int y;
	int w;
	int h;

	rect_xywh() :
			x(0), y(0), w(0), h(0) {}
	rect_xywh(const int x, const int y, const int w, const int h) :
			x(x), y(y), w(w), h(h) {}

	int area() const { return w * h; }
	int perimeter() const { return 2 * w + 2 * h; }

	auto get_wh() const {
		return rect_wh(w, h);
	}
};

struct rect_xywhf {
	int x;
	int y;
	int w;
	int h;
	bool flipped;

	rect_xywhf() :
			x(0), y(0), w(0), h(0), flipped(false) {}
	rect_xywhf(const int x, const int y, const int w, const int h, const bool flipped) :
			x(x), y(y), w(flipped ? h : w), h(flipped ? w : h), flipped(flipped) {}
	rect_xywhf(const rect_xywh &b) :
			rect_xywhf(b.x, b.y, b.w, b.h, false) {}

	int area() const { return w * h; }
	int perimeter() const { return 2 * w + 2 * h; }

	auto get_wh() const {
		return rect_wh(w, h);
	}
};

using space_rect = rect_xywh;
} //namespace rectpack2D

// insert_and_split.h
namespace rectpack2D {
struct created_splits {
	int count = 0;
	std::array<space_rect, 2> spaces;

	static auto failed() {
		created_splits result;
		result.count = -1;
		return result;
	}

	static auto none() {
		return created_splits();
	}

	template <class... Args>
	created_splits(Args &&...args) :
			spaces({ std::forward<Args>(args)... }) {
		count = sizeof...(Args);
	}

	bool better_than(const created_splits &b) const {
		return count < b.count;
	}

	explicit operator bool() const {
		return count != -1;
	}
};

inline created_splits insert_and_split(
		const rect_wh &im, /* Image rectangle */
		const space_rect &sp /* Space rectangle */
) {
	const auto free_w = sp.w - im.w;
	const auto free_h = sp.h - im.h;

	if (free_w < 0 || free_h < 0) {
		/*
			Image is bigger than the candidate empty space.
			We'll need to look further.
		*/

		return created_splits::failed();
	}

	if (free_w == 0 && free_h == 0) {
		/*
			If the image dimensions equal the dimensions of the candidate empty space (image fits exactly),
			we will just delete the space and create no splits.
		*/

		return created_splits::none();
	}

	/*
		If the image fits into the candidate empty space,
		but exactly one of the image dimensions equals the respective dimension of the candidate empty space
		(e.g. image = 20x40, candidate space = 30x40)
		we delete the space and create a single split. In this case a 10x40 space.
	*/

	if (free_w > 0 && free_h == 0) {
		auto r = sp;
		r.x += im.w;
		r.w -= im.w;
		return created_splits(r);
	}

	if (free_w == 0 && free_h > 0) {
		auto r = sp;
		r.y += im.h;
		r.h -= im.h;
		return created_splits(r);
	}

	/*
		Every other option has been exhausted,
		so at this point the image must be *strictly* smaller than the empty space,
		that is, it is smaller in both width and height.

		Thus, free_w and free_h must be positive.
	*/

	/*
		Decide which way to split.

		Instead of having two normally-sized spaces,
		it is better - though I have no proof of that - to have a one tiny space and a one huge space.
		This creates better opportunity for insertion of future rectangles.

		This is why, if we had more of width remaining than we had of height,
		we split along the vertical axis,
		and if we had more of height remaining than we had of width,
		we split along the horizontal axis.
	*/

	if (free_w > free_h) {
		const auto bigger_split = space_rect(
				sp.x + im.w,
				sp.y,
				free_w,
				sp.h);

		const auto lesser_split = space_rect(
				sp.x,
				sp.y + im.h,
				im.w,
				free_h);

		return created_splits(bigger_split, lesser_split);
	}

	const auto bigger_split = space_rect(
			sp.x,
			sp.y + im.h,
			sp.w,
			free_h);

	const auto lesser_split = space_rect(
			sp.x + im.w,
			sp.y,
			free_w,
			im.h);

	return created_splits(bigger_split, lesser_split);
}
} //namespace rectpack2D

// empty_spaces.h
namespace rectpack2D {
enum class flipping_option {
	DISABLED,
	ENABLED
};

class default_empty_spaces;

template <bool allow_flip, class empty_spaces_provider = default_empty_spaces>
class empty_spaces {
	rect_wh current_aabb;
	empty_spaces_provider spaces;

	/* MSVC fix for non-conformant if constexpr implementation */

	static auto make_output_rect(const int x, const int y, const int w, const int h) {
		return rect_xywh(x, y, w, h);
	}

	static auto make_output_rect(const int x, const int y, const int w, const int h, const bool flipped) {
		return rect_xywhf(x, y, w, h, flipped);
	}

public:
	using output_rect_type = std::conditional_t<allow_flip, rect_xywhf, rect_xywh>;

	flipping_option flipping_mode = flipping_option::ENABLED;

	empty_spaces(const rect_wh &r) {
		reset(r);
	}

	void reset(const rect_wh &r) {
		current_aabb = {};

		spaces.reset();
		spaces.add(rect_xywh(0, 0, r.w, r.h));
	}

	template <class F>
	std::optional<output_rect_type> insert(const rect_wh image_rectangle, F report_candidate_empty_space) {
		for (int i = static_cast<int>(spaces.get_count()) - 1; i >= 0; --i) {
			const auto candidate_space = spaces.get(i);

			report_candidate_empty_space(candidate_space);

			auto accept_result = [this, i, image_rectangle, candidate_space](
										 const created_splits &splits,
										 const bool flipping_necessary) -> std::optional<output_rect_type> {
				spaces.remove(i);

				for (int s = 0; s < splits.count; ++s) {
					if (!spaces.add(splits.spaces[s])) {
						return std::nullopt;
					}
				}

				if constexpr (allow_flip) {
					const auto result = make_output_rect(
							candidate_space.x,
							candidate_space.y,
							image_rectangle.w,
							image_rectangle.h,
							flipping_necessary);

					current_aabb.expand_with(result);
					return result;
				} else if constexpr (!allow_flip) {
					(void)flipping_necessary;

					const auto result = make_output_rect(
							candidate_space.x,
							candidate_space.y,
							image_rectangle.w,
							image_rectangle.h);

					current_aabb.expand_with(result);
					return result;
				}
			};

			auto try_to_insert = [&](const rect_wh &img) {
				return rectpack2D::insert_and_split(img, candidate_space);
			};

			if constexpr (!allow_flip) {
				if (const auto normal = try_to_insert(image_rectangle)) {
					return accept_result(normal, false);
				}
			} else {
				if (flipping_mode == flipping_option::ENABLED) {
					const auto normal = try_to_insert(image_rectangle);
					const auto flipped = try_to_insert(rect_wh(image_rectangle).flip());

					/*
						If both were successful,
						prefer the one that generated less remainder spaces.
					*/

					if (normal && flipped) {
						if (flipped.better_than(normal)) {
							/* Accept the flipped result if it producues less or "better" spaces. */

							return accept_result(flipped, true);
						}

						return accept_result(normal, false);
					}

					if (normal) {
						return accept_result(normal, false);
					}

					if (flipped) {
						return accept_result(flipped, true);
					}
				} else {
					if (const auto normal = try_to_insert(image_rectangle)) {
						return accept_result(normal, false);
					}
				}
			}
		}

		return std::nullopt;
	}

	decltype(auto) insert(const rect_wh &image_rectangle) {
		return insert(image_rectangle, [](auto &) {});
	}

	auto get_rects_aabb() const {
		return current_aabb;
	}

	const auto &get_spaces() const {
		return spaces;
	}
};
} //namespace rectpack2D

// empty_space_allocators.h
namespace rectpack2D {
class default_empty_spaces {
	std::vector<space_rect> empty_spaces;

public:
	void remove(const int i) {
		empty_spaces[i] = empty_spaces.back();
		empty_spaces.pop_back();
	}

	bool add(const space_rect r) {
		empty_spaces.emplace_back(r);
		return true;
	}

	auto get_count() const {
		return empty_spaces.size();
	}

	void reset() {
		empty_spaces.clear();
	}

	const auto &get(const int i) {
		return empty_spaces[i];
	}
};

template <int MAX_SPACES>
class static_empty_spaces {
	int count_spaces = 0;
	std::array<space_rect, MAX_SPACES> empty_spaces;

public:
	void remove(const int i) {
		empty_spaces[i] = empty_spaces[count_spaces - 1];
		--count_spaces;
	}

	bool add(const space_rect r) {
		if (count_spaces < static_cast<int>(empty_spaces.size())) {
			empty_spaces[count_spaces] = r;
			++count_spaces;

			return true;
		}

		return false;
	}

	auto get_count() const {
		return count_spaces;
	}

	void reset() {
		count_spaces = 0;
	}

	const auto &get(const int i) {
		return empty_spaces[i];
	}
};
} //namespace rectpack2D

// best_bin_finder.h
namespace rectpack2D {
enum class callback_result {
	ABORT_PACKING,
	CONTINUE_PACKING
};

template <class T>
auto &dereference(T &r) {
	/*
		This will allow us to pass orderings that consist of pointers,
		as well as ones that are just plain objects in a vector.
   */

	if constexpr (std::is_pointer_v<T>) {
		return *r;
	} else {
		return r;
	}
};

/*
	This function will do a binary search on viable bin sizes,
	starting from the biggest one: starting_bin.

	The search stops when the bin was successfully inserted into,
	AND the bin size to be tried next differs in size from the last viable one by *less* then discard_step.

	If we could not insert all input rectangles into a bin even as big as the starting_bin - the search fails.
	In this case, we return the amount of space (total_area_type) inserted in total.

	If we've found a viable bin that is smaller or equal to starting_bin, the search succeeds.
	In this case, we return the viable bin (rect_wh).
*/

enum class bin_dimension {
	BOTH,
	WIDTH,
	HEIGHT
};

template <class empty_spaces_type, class O>
std::variant<total_area_type, rect_wh> best_packing_for_ordering_impl(
		empty_spaces_type &root,
		O ordering,
		const rect_wh starting_bin,
		int discard_step,
		const bin_dimension tried_dimension) {
	auto candidate_bin = starting_bin;
	int tries_before_discarding = 0;

	if (discard_step <= 0) {
		tries_before_discarding = -discard_step;
		discard_step = 1;
	}

	//std::cout << "best_packing_for_ordering_impl dim: " << int(tried_dimension) << " w: " << starting_bin.w << " h: " << starting_bin.h << std::endl;

	int starting_step = 0;

	if (tried_dimension == bin_dimension::BOTH) {
		candidate_bin.w /= 2;
		candidate_bin.h /= 2;

		starting_step = candidate_bin.w / 2;
	} else if (tried_dimension == bin_dimension::WIDTH) {
		candidate_bin.w /= 2;
		starting_step = candidate_bin.w / 2;
	} else {
		candidate_bin.h /= 2;
		starting_step = candidate_bin.h / 2;
	}

	for (int step = starting_step;; step = std::max(1, step / 2)) {
		//std::cout << "candidate: " << candidate_bin.w << "x" << candidate_bin.h << std::endl;

		root.reset(candidate_bin);

		int total_inserted_area = 0;

		const bool all_inserted = [&]() {
			for (const auto &r : ordering) {
				const auto &rect = dereference(r);

				if (root.insert(rect.get_wh())) {
					total_inserted_area += rect.area();
				} else {
					return false;
				}
			}

			return true;
		}();

		if (all_inserted) {
			/* Attempt was successful. Try with a smaller bin. */

			if (step <= discard_step) {
				if (tries_before_discarding > 0) {
					tries_before_discarding--;
				} else {
					return candidate_bin;
				}
			}

			if (tried_dimension == bin_dimension::BOTH) {
				candidate_bin.w -= step;
				candidate_bin.h -= step;
			} else if (tried_dimension == bin_dimension::WIDTH) {
				candidate_bin.w -= step;
			} else {
				candidate_bin.h -= step;
			}

			root.reset(candidate_bin);
		} else {
			/* Attempt ended with failure. Try with a bigger bin. */

			if (tried_dimension == bin_dimension::BOTH) {
				candidate_bin.w += step;
				candidate_bin.h += step;

				if (candidate_bin.area() > starting_bin.area()) {
					return total_inserted_area;
				}
			} else if (tried_dimension == bin_dimension::WIDTH) {
				candidate_bin.w += step;

				if (candidate_bin.w > starting_bin.w) {
					return total_inserted_area;
				}
			} else {
				candidate_bin.h += step;

				if (candidate_bin.h > starting_bin.h) {
					return total_inserted_area;
				}
			}
		}
	}
}

template <class empty_spaces_type, class O>
std::variant<total_area_type, rect_wh> best_packing_for_ordering(
		empty_spaces_type &root,
		O &&ordering,
		const rect_wh starting_bin,
		const int discard_step) {
	const auto try_pack = [&](
								  const bin_dimension tried_dimension,
								  const rect_wh starting_bin) {
		return best_packing_for_ordering_impl(
				root,
				std::forward<O>(ordering),
				starting_bin,
				discard_step,
				tried_dimension);
	};

	const auto best_result = try_pack(bin_dimension::BOTH, starting_bin);

	if (const auto failed = std::get_if<total_area_type>(&best_result)) {
		return *failed;
	}

	auto best_bin = std::get<rect_wh>(best_result);

	auto trial = [&](const bin_dimension tried_dimension) {
		const auto trial = try_pack(tried_dimension, best_bin);

		if (const auto better = std::get_if<rect_wh>(&trial)) {
			best_bin = *better;
		}
	};

	trial(bin_dimension::WIDTH);
	trial(bin_dimension::HEIGHT);

	return best_bin;
}

/*
	This function will try to find the best bin size among the ones generated by all provided rectangle orders.
	Only the best order will have results written to.

	The function reports which of the rectangles did and did not fit in the end.
*/

template <
		class empty_spaces_type,
		class OrderType,
		class F,
		class I>
rect_wh find_best_packing_impl(F for_each_order, const I input) {
	const auto max_bin = rect_wh(input.max_bin_side, input.max_bin_side);

	OrderType *best_order = nullptr;

	int best_total_inserted = -1;
	auto best_bin = max_bin;

	/*
		The root node is re-used on the TLS.
		It is always reset before any packing attempt.
	*/

	thread_local empty_spaces_type root = rect_wh();
	root.flipping_mode = input.flipping_mode;

	for_each_order([&](OrderType &current_order) {
		const auto packing = best_packing_for_ordering(
				root,
				current_order,
				max_bin,
				input.discard_step);

		if (const auto total_inserted = std::get_if<total_area_type>(&packing)) {
			/*
				Track which function inserts the most area in total,
				just in case that all orders will fail to fit into the largest allowed bin.
			*/
			if (best_order == nullptr) {
				if (*total_inserted > best_total_inserted) {
					best_order = std::addressof(current_order);
					best_total_inserted = *total_inserted;
				}
			}
		} else if (const auto result_bin = std::get_if<rect_wh>(&packing)) {
			/* Save the function if it performed the best. */
			if (result_bin->area() <= best_bin.area()) {
				best_order = std::addressof(current_order);
				best_bin = *result_bin;
			}
		}
	});

	{
		// assert(best_order != nullptr);

		root.reset(best_bin);

		for (auto &rr : *best_order) {
			auto &rect = dereference(rr);

			if (const auto ret = root.insert(rect.get_wh())) {
				rect = *ret;

				if (callback_result::ABORT_PACKING == input.handle_successful_insertion(rect)) {
					break;
				}
			} else {
				if (callback_result::ABORT_PACKING == input.handle_unsuccessful_insertion(rect)) {
					break;
				}
			}
		}

		return root.get_rects_aabb();
	}
}
} //namespace rectpack2D

// finders_interface.h
namespace rectpack2D {
template <class empty_spaces_type>
using output_rect_t = typename empty_spaces_type::output_rect_type;

template <class F, class G>
struct finder_input {
	const int max_bin_side;
	const int discard_step;
	F handle_successful_insertion;
	G handle_unsuccessful_insertion;
	const flipping_option flipping_mode;
};

template <class F, class G>
auto make_finder_input(
		const int max_bin_side,
		const int discard_step,
		F &&handle_successful_insertion,
		G &&handle_unsuccessful_insertion,
		const flipping_option flipping_mode) {
	return finder_input<F, G>{
		max_bin_side,
		discard_step,
		std::forward<F>(handle_successful_insertion),
		std::forward<G>(handle_unsuccessful_insertion),
		flipping_mode
	};
};

/*
	Finds the best packing for the rectangles,
	just in the order that they were passed.
*/

template <class empty_spaces_type, class F, class G>
rect_wh find_best_packing_dont_sort(
		std::vector<output_rect_t<empty_spaces_type>> &subjects,
		const finder_input<F, G> &input) {
	using order_type = std::remove_reference_t<decltype(subjects)>;

	return find_best_packing_impl<empty_spaces_type, order_type>(
			[&subjects](auto callback) { callback(subjects); },
			input);
}

/*
	Finds the best packing for the rectangles.
	Accepts a list of predicates able to compare two input rectangles.

	The function will try to pack the rectangles in all orders generated by the predicates,
	and will only write the x, y coordinates of the best packing found among the orders.
*/

template <class empty_spaces_type, class F, class G, class Comparator, class... Comparators>
rect_wh find_best_packing(
		std::vector<output_rect_t<empty_spaces_type>> &subjects,
		const finder_input<F, G> &input,

		Comparator comparator,
		Comparators... comparators) {
	using rect_type = output_rect_t<empty_spaces_type>;
	using order_type = std::vector<rect_type *>;

	constexpr auto count_orders = 1 + sizeof...(Comparators);
	thread_local std::array<order_type, count_orders> orders;

	{
		/* order[0] will always exist since this overload requires at least one comparator */
		auto &initial_pointers = orders[0];
		initial_pointers.clear();

		for (auto &s : subjects) {
			if (s.area() > 0) {
				initial_pointers.emplace_back(std::addressof(s));
			}
		}

		for (std::size_t i = 1; i < count_orders; ++i) {
			orders[i] = initial_pointers;
		}
	}

	std::size_t f = 0;

	auto &orders_ref = orders;

	auto make_order = [&f, &orders_ref](auto &predicate) {
		std::sort(orders_ref[f].begin(), orders_ref[f].end(), predicate);
		++f;
	};

	make_order(comparator);
	(make_order(comparators), ...);

	return find_best_packing_impl<empty_spaces_type, order_type>(
			[&orders_ref](auto callback) { for (auto& o : orders_ref) { callback(o); } },
			input);
}

/*
	Finds the best packing for the rectangles.
	Provides a list of several sensible comparison predicates.
*/

template <class empty_spaces_type, class F, class G>
rect_wh find_best_packing(
		std::vector<output_rect_t<empty_spaces_type>> &subjects,
		const finder_input<F, G> &input) {
	using rect_type = output_rect_t<empty_spaces_type>;

	return find_best_packing<empty_spaces_type>(
			subjects,
			input,

			[](const rect_type *const a, const rect_type *const b) {
				return a->area() > b->area();
			},
			[](const rect_type *const a, const rect_type *const b) {
				return a->perimeter() > b->perimeter();
			},
			[](const rect_type *const a, const rect_type *const b) {
				return std::max(a->w, a->h) > std::max(b->w, b->h);
			},
			[](const rect_type *const a, const rect_type *const b) {
				return a->w > b->w;
			},
			[](const rect_type *const a, const rect_type *const b) {
				return a->h > b->h;
			},
			[](const rect_type *const a, const rect_type *const b) {
				return a->get_wh().pathological_mult() > b->get_wh().pathological_mult();
			});
}
} //namespace rectpack2D

void ImagePacker::_bind_methods() {
	ClassDB::bind_static_method("ImagePacker",
			D_METHOD("pack", "images", "format", "meta_name"),
			&ImagePacker::pack);
}

Ref<Image> ImagePacker::pack(
		TypedArray<Image> images,
		Image::Format format,
		StringName meta_name) {
	using namespace rectpack2D;

	using spaces_type = rectpack2D::empty_spaces<false>;
	using rect_type = output_rect_t<spaces_type>;

	auto report_successful = [](rect_type &) {
		return callback_result::CONTINUE_PACKING;
	};

	auto report_unsuccessful = [](rect_type &) {
		return callback_result::ABORT_PACKING;
	};

	std::vector<rect_type> rectangles;
	for (int i = 0; i < images.size(); i++) {
		const Ref<Image> &image = images[i];
		rectangles.emplace_back(rect_xywh(
				0,
				0,
				image->get_width(),
				image->get_height()));
	}

	const auto result_size = find_best_packing<spaces_type>(
			rectangles,
			make_finder_input(
					4096,
					1,
					report_successful,
					report_unsuccessful,
					flipping_option::DISABLED));

	Ref<Image> output_image = Image::create_empty(
			result_size.w,
			result_size.h,
			false,
			format);

	for (int i = 0; i < images.size(); i++) {
		const Ref<Image> &image = images[i];
		const rect_type &rect = rectangles[i];

		Vector2i image_position = Vector2i(rect.x, rect.y);

		image->set_meta(meta_name, image_position);
		output_image->blit_rect(
				image,
				Rect2i(0, 0, rect.w, rect.h),
				image_position);
	}

	return output_image;
}

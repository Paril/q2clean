#pragma once

#include "config.h"
#include "lib/types.h"
#include "lib/math/vector.h"
#include "lib/std.h"

// randomness!

namespace internal
{
	extern std::mt19937_64 rng;
};

// return a random unsigned integer between [0, std::numeric_limits<uint64_t>::max()]
[[nodiscard]] inline uint64_t Q_rand()
{
	return internal::rng();
}

template<std::integral T, std::integral U>
struct copy_signedness
{
	using type = std::conditional_t<std::is_signed_v<T>, std::make_signed_t<U>, std::make_unsigned_t<U>>;
};

// return a random unsigned integer between [min, max).
// if max is <= min, always returns min.
template<std::integral T = int64_t>
[[nodiscard]] inline T Q_rand_uniform(const T &min, const T &max)
{
	if (min == std::numeric_limits<T>::max() || max <= min + 1)
		return min;

	// resolves constraint N4659 29.6.1.1
	if constexpr(std::is_same_v<std::make_signed_t<T>, int8_t>)
		return (T) std::uniform_int_distribution<copy_signedness<T, int16_t>::type>(min, max - 1)(internal::rng);
	else
		return std::uniform_int_distribution<T>(min, max - 1)(internal::rng);
}

// return a random boolean value
[[nodiscard]] inline bool Q_rand_bool()
{
	return Q_rand_uniform(0, 2) == 0;
}

// return a random unsigned integer between [0, max).
// if max is zero, always returns 0.
template<std::integral T = int64_t>
[[nodiscard]] inline T Q_rand_uniform(const T &max)
{
	return Q_rand_uniform<T>(0, max);
}

// return a random float between [min, max)
template<std::floating_point T = float>
[[nodiscard]] inline T random(const T &min, const T &max)
{
	return std::uniform_real_distribution<T>(min, max)(internal::rng);
}

// return a random float between [0, max)
template<std::floating_point T = float>
[[nodiscard]] inline T random(const T &max)
{
	return random(0.f, max);
}

// return a random float between [0, 1)
template<std::floating_point T = float>
[[nodiscard]] inline T random()
{
	return random(0.f, 1.f);
}

// random time points
template<std::integral rep, typename period>
[[nodiscard]] inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &min, const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(Q_rand_uniform(min.count(), max.count()));
}

// random time points
template<std::integral rep, typename period>
[[nodiscard]] inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(Q_rand_uniform(max.count()));
}

// random time points
template<std::floating_point rep, typename period>
[[nodiscard]] inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &min, const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(random(min.count(), max.count()));
}

// random time points
template<std::floating_point rep, typename period>
[[nodiscard]] inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(random(max.count()));
}

// random time points
template<typename rep, typename period>
inline std::chrono::duration<rep, period> crandom(const std::chrono::duration<rep, period> &v)
{
	return random(-v, v);
}

// return a random vector between [0, 1)
inline vector randomv()
{
	return { random(), random(), random() };
}

// return a random vector between [0, max)
inline vector randomv(const vector &max)
{
	return { random(max.x), random(max.y), random(max.z) };
}

// return a random vector between [min, max)
inline vector randomv(const vector &min, const vector &max)
{
	return { random(min.x, max.x), random(min.y, max.y), random(min.z, max.z) };
}

// pick a random value from the specified arguments
template<typename T>
inline auto random_of(const std::initializer_list<T> &args)
{
	return *(args.begin() + Q_rand_uniform(args.size()));
}

// pick a random value from the specified array
template<typename T, size_t N>
inline auto random_of(const T (&args)[N])
{
	return args[Q_rand_uniform(lengthof(args))];
}

// random float between -v and v
inline float crandom(const float &v)
{
	return random(-v, v);
}

// random float between -1.f and 1.f
inline float crandom()
{
	return crandom(1.f);
}

// random vector between { -1, -1, -1 } - { 1, 1, 1 }
inline vector crandomv(const vector &v)
{
	return randomv(-v, v);
}

inline vector crandomv()
{
	return crandomv({ 1, 1, 1 });
}

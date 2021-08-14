#pragma once

#include "config.h"
#include "lib/types.h"
#include "lib/math/vector.h"
#include <random>
#include <chrono>

// randomness!

namespace internal
{
	extern std::mt19937_64 rng;
};

// return a random unsigned integer between [0, std::numeric_limits<uint64_t>::max()]
inline uint64_t Q_rand()
{
	return internal::rng();
}

#include <random>
#include <concepts>

// return a random unsigned integer between [min, max).
// if max is <= min, always returns min.
template<std::integral T = int64_t>
inline T Q_rand_uniform(const T &min, const T &max)
{
	if (max <= min + 1)
		return min;

	return std::uniform_int_distribution<T>(min, max - 1)(internal::rng);
}

// return a random unsigned integer between [0, max).
// if max is zero, always returns 0.
template<std::integral T = int64_t>
inline T Q_rand_uniform(const T &max)
{
	if constexpr (std::is_same_v<std::make_signed_t<T>, int8_t>)
		return (T) Q_rand_uniform((int16_t) 0, (int16_t) max);
	else
		return Q_rand_uniform((T) 0, max);
}

// return a random float between [0, 1)
template<std::floating_point T = float>
inline T random()
{
	return std::uniform_real_distribution<T>()(internal::rng);
}

// return a random float between [0, max)
template<std::floating_point T = float>
inline T random(const T &max)
{
	return std::uniform_real_distribution<T>((T) 0, max)(internal::rng);
}

// return a random float between [min, max)
template<std::floating_point T = float>
inline T random(const T &min, const T &max)
{
	return std::uniform_real_distribution<T>(min, max)(internal::rng);
}

// random time points
template<std::integral rep, typename period>
inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &min, const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(Q_rand_uniform(min.count(), max.count()));
}

// random time points
template<std::integral rep, typename period>
inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(Q_rand_uniform(max.count()));
}

// random time points
template<std::floating_point rep, typename period>
inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &min, const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(random(min.count(), max.count()));
}

// random time points
template<std::floating_point rep, typename period>
inline std::chrono::duration<rep, period> random(const std::chrono::duration<rep, period> &max)
{
	return std::chrono::duration<rep, period>(random(max.count()));
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

inline float crandom()
{
	return random(-1.f, 1.f);
}

inline vector crandomv()
{
	return randomv({ -1, -1, -1 }, { 1, 1, 1 });
}

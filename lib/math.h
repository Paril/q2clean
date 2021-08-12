#pragma once 

#include <cmath>

// specialized templates for certain math-like algorithms/operations

// returns the lowest of a or b
template<typename T>
[[nodiscard]] constexpr const T &min(const T &a, const T &b)
{
	return a < b ? a : b;
}

// returns the highest of a or b
template<typename T>
[[nodiscard]] constexpr const T &max(const T &a, const T &b)
{
	return a > b ? a : b;
}

// clamp value between min and max (min <= value <= max)
template<typename T>
[[nodiscard]] constexpr const T &clamp(const T &min, const T &value, const T &max)
{
	return value < min ? min : value > max ? max : value;
}

// a fast method of doing a Euler angular clamp (angle % 360)
[[nodiscard]] constexpr float anglemod(const float &degrees)
{
	return (360.0f / 65536) * ((int32_t) (degrees * (65536 / 360.0f)) & 65535);
}

// pie
constexpr float PI = (float) 3.14159265358979323846;

// convert degrees to radians
[[nodiscard]] constexpr float deg2rad(const float &a)
{
	return a * (PI / 180);
}

// convert radians to degrees
[[nodiscard]] constexpr float rad2deg(const float &a)
{
	return a * (180 / PI);
}

// allows lengthof(array) for C-style arrays
template<typename T, size_t S>
inline size_t lengthof(T(&)[S])
{
	return S;
}

// takes inputs that can be convertable to booleans and
// constructs an integer out of them.
template<std::convertible_to<bool> ...Args>
constexpr auto boolbits(Args... args)
{
	static_assert(sizeof...(args) <= 64, "Too many booleans to encode into bits");

	uint8_t index = 0;

	if constexpr (sizeof...(args) > 32)
	{
		int64_t bits = 0;

		((bits |= args ? 1ull << index : 0, index++), ...);

		return bits;
	}
	else
	{
		int32_t bits = 0;

		((bits |= args ? 1 << index : 0, index++), ...);

		return bits;
	}
}
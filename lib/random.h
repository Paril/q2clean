#pragma once

#include "types.h"

// randomness!

// return a random unsigned integer between [0, UINT_MAX], inclusive
uint32_t Q_rand();

// return a random unsigned integer between [0, max), exclusive.
// if max is zero, always returns 0.
uint32_t Q_rand_uniform(const uint32_t &max);

// return a random float between [0, 1), exclusive
float random();

// return a random float between [0, max), exclusive
float random(const float &max);

// return a random float between [min, max), exclusive
float random(const float &min, const float &max);

#include "vector.h"

// return a random vector between [0, 1), exclusive
vector randomv();

// return a random vector between [0, max), exclusive
vector randomv(const vector &max);

// return a random vector between [min, max), exclusive
vector randomv(const vector &min, const vector &max);

inline float crandom()
{
	return random(-1.f, 1.f);
}

inline vector crandomv()
{
	return randomv({ -1, -1, -1 }, { 1, 1, 1 });
}
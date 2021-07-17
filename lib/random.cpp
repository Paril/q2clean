#include "random.h"
#include <random>
#include <ctime>

static std::mt19937 gen((uint32_t)time(nullptr));

uint32_t Q_rand()
{
	return gen();
}

uint32_t Q_rand_uniform(const uint32_t &max)
{
	if (max <= 1)
		return 0;

	return std::uniform_int_distribution<uint32_t>(0, max - 1)(gen);
}

float random()
{
	return std::uniform_real_distribution<float>()(gen);
}

float random(const float &max)
{
	return std::uniform_real_distribution<float>(0.f, max)(gen);
}

float random(const float &min, const float &max)
{
	return std::uniform_real_distribution<float>(min, max)(gen);
}

vector randomv()
{
	return { random(), random(), random() };
}

vector randomv(const vector &max)
{
	return { random(max.x), random(max.y), random(max.z) };
}

vector randomv(const vector &min, const vector &max)
{
	return { random(min.x, max.x), random(min.y, max.y), random(min.z, max.z) };
}
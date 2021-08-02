#pragma once

#include "vector.h"
#include <cfloat>

// a structure that holds an axis-aligned bounding box.
// by default, this is a point (all zeroes) to match Q2's
// usual expectations. for a truly empty bounds, use bounds_empty.
struct bbox
{
	vector mins = { 0, 0, 0 }, maxs = { 0, 0, 0 };

	// add point to bounds
	constexpr bbox operator+(const vector &v) const
	{
		return {
			.mins = {
				min(mins[0], v[0]),
				min(mins[1], v[1]),
				min(mins[2], v[2])
			},
			.maxs = {
				max(maxs[0], v[0]),
				max(maxs[1], v[1]),
				max(maxs[2], v[2])
			}
		};
	}

	constexpr bbox &operator+=(const vector &v)
	{
		return *this = *this + v;
	}

	// add bounds to bounds
	constexpr bbox operator+(const bbox &b) const
	{
		return (*this + b.mins) + b.maxs;
	}

	constexpr bbox &operator+=(const bbox &b)
	{
		return *this = *this + b;
	}

	constexpr vector center() const
	{
		return (mins + maxs) * 0.5f;
	}

	constexpr bbox offsetted(const vector &v) const
	{
		return { .mins = mins + v, .maxs = maxs + v };
	}

	// construct a bbox of size { -v, -v, -v } to { v, v, v }
	static constexpr bbox sized(const float &v)
	{
		return bbox { .mins = { -v, -v, -v }, .maxs = { v, v, v } };
	}

	// construct a bbox of size { -vx, -vy, -vz } to { vx, vy, vz }
	static constexpr bbox sized(const vector &v)
	{
		return bbox { .mins = -v, .maxs = v };
	}
};

// a bbox that has an infinitely empty size
constexpr bbox bbox_empty = { .mins = { INFINITY, INFINITY, INFINITY }, .maxs = { -INFINITY, -INFINITY, -INFINITY } };
constexpr bbox bbox_point;
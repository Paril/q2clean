#pragma once

#include "types.h"
#include "surface_flags.h"

struct surface
{
public:
	surface();

	// surface name
	stringarray<16>	name;
	// flags of this surface
	surface_flags	flags;
	// value for light surfaces
	int32_t			value;
};

extern const surface null_surface;
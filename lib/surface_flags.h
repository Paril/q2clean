#pragma once

#include "types.h"

enum surface_flags : uint32_t
{
	SURF_NONE = 0,
	// value will hold the light strength
	SURF_LIGHT = 1 << 0,
	// icey surface
	SURF_SLICK = 1 << 1,
	// don't draw, but add to skybox
	SURF_SKY = 1 << 2,
	// turbulent water warp
	SURF_WARP = 1 << 3,
	SURF_TRANS33 = 1 << 4,
	SURF_TRANS66 = 1 << 5,
	// scroll towards angle
	SURF_FLOWING = 1 << 6,
	// don't bother referencing the texture
	SURF_NODRAW = 1 << 7,
	// used by kmquake2 for alphatest-y surfaces
	SURF_ALPHATEST	= 1 << 25
};
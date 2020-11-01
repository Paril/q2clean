#pragma once

#include "../lib/types.h"

// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
enum entity_effects : uint32_t
{
	EF_NONE,
	EF_ROTATE = 1 << 0,	// rotate (bonus items)
	EF_GIB = 1 << 1,		// leave a trail
	EF_UNUSED = 1 << 2,
	EF_BLASTER = 1 << 3,	// redlight + trail
	EF_ROCKET = 1 << 4,	// redlight + trail
	EF_GRENADE = 1 << 5,
	EF_HYPERBLASTER = 1 << 6,
	EF_BFG = 1 << 7,
	EF_COLOR_SHELL = 1 << 8,
	EF_POWERSCREEN = 1 << 9,
	EF_ANIM01 = 1 << 10,		// automatically cycle between frames 0 and 1 at 2 hz
	EF_ANIM23 = 1 << 11,		// automatically cycle between frames 2 and 3 at 2 hz
	EF_ANIM_ALL = 1 << 12,		// automatically cycle through all frames at 2hz
	EF_ANIM_ALLFAST = 1 << 13,	// automatically cycle through all frames at 10hz
	EF_FLIES = 1 << 14,
	EF_QUAD = 1 << 15,
	EF_PENT = 1 << 16,
	EF_TELEPORTER = 1 << 17,		// particle fountain
	EF_FLAG1 = 1 << 18,
	EF_FLAG2 = 1 << 19,
// RAFAEL
	EF_IONRIPPER = 1 << 20,
	EF_GREENGIB = 1 << 21,
	EF_BLUEHYPERBLASTER = 1 << 22,
	EF_SPINNINGLIGHTS = 1 << 23,
	EF_PLASMA = 1 << 24,
	EF_TRAP = 1 << 25,
// RAFAEL

//ROGUE
	EF_TRACKER = 1 << 26,
	EF_DOUBLE = 1 << 27,
	EF_SPHERETRANS = 1 << 28,
	EF_TAGTRAIL = 1 << 29,
	EF_HALF_DAMAGE = 1 << 30,
//ROGUE

	EF_TRACKERTRAIL = 1u << 31
};

MAKE_ENUM_BITWISE(entity_effects);
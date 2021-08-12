#pragma once

#include "lib/types.h"
#include <type_traits>

// Time is stored as an unsigned 64-bit integer
using gtimediff = int64_t;
using gtime = uint64_t;

// default server FPS
constexpr gtime		BASE_FRAMERATE = 10;
// the amount of time, in s, that a single frame lasts for
constexpr float		BASE_1_FRAMETIME = 1.0f / BASE_FRAMERATE;
// the amount of time, in ms, that a single frame lasts for
constexpr gtime		BASE_FRAMETIME = BASE_1_FRAMETIME * 1000;
// the amount of time, in s, that a single frame lasts for
constexpr float		BASE_FRAMETIME_1000 = BASE_1_FRAMETIME;
// the amount of time, in s, that a single frame lasts for
constexpr float		FRAMETIME = BASE_1_FRAMETIME;

// Means of death structure for structured death
// messages. Formatter arguments:
// 0 - string, killee's name
// 1 - string, killee's gendered "their" string
// 2 - string, killee's gendered "themself" string
// 3 - string, killer's name, other_kill_fmt only
struct means_of_death
{
	// The format to use if we are also the attacker.
	// This is also the fallback if other_kill_fmt is null.
	// If this is null, it will fall back to MOD_DEFAULT's
	// kill format.
	stringlit self_kill_fmt = nullptr;

	// The format to use if something else killed us.
	// This can be left null to only use self-kill formats.
	stringlit other_kill_fmt = nullptr;
};

// a const ref to a means_of_death. We pass these around
// to prevent copying.
using means_of_death_ref = const means_of_death &;
#pragma once

#include "lib/std.h"
#include "lib/types.h"

using gtime = std::chrono::milliseconds;
using gtimef = std::chrono::duration<double>;

constexpr uint64_t	framerate = 10;
// server framerate
// server frame time, in seconds
constexpr gtimef	frametime_s = gtimef(1.0f / framerate);

using namespace std::chrono;
using namespace std::chrono_literals;
using std::ratio;

// type to describe a single game frame
using frames = duration<int64_t, ratio<1, framerate>>;

// literal to return frame counts; generally you don't
// want to use this.
constexpr frames operator""_hz(uint64_t f)
{
	return frames(f);
}
// server frame time, in ms
constexpr milliseconds	framerate_ms = duration_cast<milliseconds>(frametime_s);

// default server FPS
constexpr uint64_t		BASE_FRAMERATE = framerate;
// the amount of time, in s, that a single frame lasts for
constexpr gtimef		BASE_1_FRAMETIME = frametime_s;
// the amount of time, in ms, that a single frame lasts for
constexpr milliseconds	BASE_FRAMETIME = framerate_ms;
// the amount of time, in s, that a single frame lasts for
constexpr gtimef		BASE_FRAMETIME_1000 = BASE_1_FRAMETIME;
// the amount of time, in s, that a single frame lasts for
constexpr gtimef		FRAMETIME = BASE_1_FRAMETIME;

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
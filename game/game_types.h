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

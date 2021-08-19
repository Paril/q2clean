#pragma once

#include "lib/std.h"
#include "lib/types/allocator.h"

// set is an allocator-wrapped unordered_set
template<typename T>
using set = std::unordered_set<T, game_allocator<T>>;
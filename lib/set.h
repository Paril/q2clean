#pragma once

#include "allocator.h"
#include <unordered_set>

// set is an allocator-wrapped unordered_set
template<typename T>
using set = std::unordered_set<T, game_allocator<T>>;
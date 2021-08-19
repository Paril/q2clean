#pragma once

#include "lib/std.h"
#include "lib/types/allocator.h"

// dynarray is the name used by Q2++ for what C++ calls
// a "vector", since "vector" is used as 3d vector
template<typename T>
using dynarray = std::vector<T, game_allocator<T>>;
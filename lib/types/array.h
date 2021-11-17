#pragma once

// array is used for some built-in stuff
#include "lib/std.h"
#include "lib/types.h"

template<typename TKey, size_t N>
using array = std::array<TKey, N>;

template<size_t N>
using bitset = std::bitset<N>;
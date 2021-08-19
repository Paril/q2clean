#pragma once

#include "lib/std.h"
#include "lib/types/allocator.h"

// map is an allocator-wrapped unordered_map
template<typename TKey, typename TVal>
using map = std::unordered_map<TKey, TVal, std::hash<TKey>, std::equal_to<TKey>, game_allocator<std::pair<const TKey, TVal>>>;
#pragma once

#include "allocator.h"
#include <unordered_map>

// set is an allocator-wrapped unordered_set
template<typename TKey, typename TVal>
using map = std::unordered_map<TKey, TVal, std::hash<TKey>, std::equal_to<TKey>, game_allocator<std::pair<const TKey, TVal>>>;
#pragma once

// array is used for some built-in stuff
#include <array>
#include "lib/types.h"

template<typename TKey, size_t N>
using array = std::array<TKey, N>;
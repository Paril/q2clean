#pragma once

// array is used for some built-in stuff
import <array>;
import "lib/types.h";

template<typename TKey, size_t N>
using array = std::array<TKey, N>;
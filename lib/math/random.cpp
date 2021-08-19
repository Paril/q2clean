#include "config.h"
#include "lib/std.h"
#include "lib/types.h"
#include "lib/math/vector.h"
#include "random.h"

namespace internal
{
	std::mt19937_64 rng((uint64_t) _time64(nullptr));
};

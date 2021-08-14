#include "config.h"
#include <ctime>
#include "lib/types.h"
#include "lib/math/vector.h"
#include "random.h"

namespace internal
{
	std::mt19937_64 rng((uint64_t) time(nullptr));
};

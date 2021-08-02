#include "config.h"

#ifdef THE_RECKONING
#include "game/entity_types.h"
#include "lib/types.h"
#include "lib/math/vector.h"

void fire_trap(entity &self, vector start, vector aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held);
#endif
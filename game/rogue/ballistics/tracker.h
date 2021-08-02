#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"

void fire_tracker(entity &self, vector start, vector dir, int32_t damage, int32_t cspeed, entityref cenemy);
#endif
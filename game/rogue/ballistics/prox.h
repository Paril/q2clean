#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"

void fire_prox(entity &self, vector start, vector aimdir, int32_t dmg_multiplier, int32_t speed);
#endif
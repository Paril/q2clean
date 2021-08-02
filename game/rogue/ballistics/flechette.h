#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"

void fire_flechette(entity &self, vector start, vector dir, int32_t damage, int32_t speed, int32_t kick);
#endif
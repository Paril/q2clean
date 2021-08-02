#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"
#include "game/spawn.h"

constexpr float TESLA_DAMAGE_RADIUS = 128.f;

void fire_tesla(entity &self, vector start, vector aimdir, int32_t damage_multiplier, int32_t speed);

DECLARE_ENTITY(TESLA);
#endif
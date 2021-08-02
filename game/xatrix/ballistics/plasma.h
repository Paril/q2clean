#pragma once

#include "config.h"

#ifdef THE_RECKONING
#include "game/entity_types.h"
#include "lib/protocol.h"

void fire_plasma(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage);
#endif
#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "lib/types.h"
#include "lib/math/vector.h"

void fire_bfg(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius);
#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"

/*
=================
fire_heat

Fires a single heat beam.  Zap.
=================
*/
void fire_heatbeam(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, bool monster);
#endif
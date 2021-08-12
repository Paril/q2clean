#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity_types.h"
#include "lib/math/vector.h"
#include "game/combat.h"

void fire_player_melee(entity &self, vector start, vector aim, int32_t reach, int32_t damage, int32_t kick, means_of_death_ref mod);
#endif
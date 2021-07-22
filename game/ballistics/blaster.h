#pragma once

#include "config.h"
#include "game/entity_types.h"
#include "lib/protocol.h"
#include "game/combat.h"
#include "lib/types.h"

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, means_of_death mod, bool hyper);
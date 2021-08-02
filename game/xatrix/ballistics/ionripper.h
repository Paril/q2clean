#pragma once

#include "config.h"

#ifdef THE_RECKONING
#include "game/entity_types.h"
#include "lib/protocol.h"

void fire_ionripper(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect);
#endif
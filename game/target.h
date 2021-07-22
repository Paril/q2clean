#pragma once

#include "../lib/types.h"
#include "game/entity_types.h"
#include "lib/savables.h"

void target_laser_think(entity &self);

DECLARE_SAVABLE_FUNCTION(target_laser_think);
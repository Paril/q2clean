#pragma once

import "../lib/types.h";
import "game/entity_types.h";
import "lib/savables.h";

void target_laser_think(entity &self);

DECLARE_SAVABLE_FUNCTION(target_laser_think);
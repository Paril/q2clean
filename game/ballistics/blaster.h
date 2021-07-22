#pragma once

import "config.h";
import "game/entity_types.h";
import "lib/protocol.h";
import "game/combat.h";
import "lib/types.h";

void fire_blaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity_effects effect, means_of_death mod, bool hyper);
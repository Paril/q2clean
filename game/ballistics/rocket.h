#pragma once

import "lib/types.h";
import "game/entity_types.h";
import "lib/math/vector.h";

entity &fire_rocket(entity &self, vector start, vector dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage);

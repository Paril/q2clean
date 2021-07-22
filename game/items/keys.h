#pragma once

import "config.h";

#if defined(SINGLE_PLAYER)
import "game/entity_types.h";

bool Pickup_Key(entity &ent, entity &other);
#endif
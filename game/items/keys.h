#pragma once

#include "config.h"

#if defined(SINGLE_PLAYER)
#include "game/entity_types.h"

bool Pickup_Key(entity &ent, entity &other);
#endif
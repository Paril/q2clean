#include "config.h"
#include "game/entity_types.h"

bool Pickup_Adrenaline(entity &ent, entity &other);

bool Pickup_AncientHead(entity &ent, entity &other);

void MegaHealth_think(entity &self);

// special values for health styles
constexpr spawn_flag HEALTH_IGNORE_MAX = (spawn_flag) 1;
constexpr spawn_flag HEALTH_TIMED = (spawn_flag) 2;

bool Pickup_Health(entity &ent, entity &other);
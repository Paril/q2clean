#pragma once

#include "config.h"
#include "game/entity_types.h"

bool Pickup_Adrenaline(entity &ent, entity &other);

bool Pickup_AncientHead(entity &ent, entity &other);

void MegaHealth_think(entity &self);

bool Pickup_Health(entity &ent, entity &other);
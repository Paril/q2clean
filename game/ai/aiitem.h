#pragma once

#include "../../lib/types.h"

struct gitem_t;

bool AI_CanPick_Ammo(entity &ent, const gitem_t &it);

float AI_ItemWeight(entity &self, entity &it);

bool AI_ItemIsReachable(entity &self, vector goal);

bool AI_CanUseArmor(const gitem_t &it, entity &other);

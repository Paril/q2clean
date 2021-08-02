#pragma once

#include "config.h"

#ifdef THE_RECKONING
#include "game/entity_types.h"
#include "lib/math/vector.h"
#include "lib/protocol.h"

void monster_fire_blueblaster(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype, entity_effects effect);

void monster_fire_ionripper(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype, entity_effects effect);

void monster_fire_heat(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype);

void monster_dabeam(entity &self);
#endif
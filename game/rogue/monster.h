#pragma once

#include "config.h"

#ifdef GROUND_ZERO
#include "lib/protocol.h"
#include "game/entity_types.h"
#include "game/misc.h"

void monster_fire_blaster2(entity &self, vector start, vector dir, int32_t damage, int32_t speed, monster_muzzleflash flashtype, entity_effects effect);

void monster_fire_tracker(entity &self, vector start, vector dir, int32_t damage, int32_t speed, entity enemy, monster_muzzleflash flashtype);

void monster_fire_heatbeam(entity &self, vector start, vector dir, int32_t damage, int32_t kick, monster_muzzleflash flashtype);

entity &CreateMonster(vector origin, vector angles, const entity_type &type);

bool CheckSpawnPoint(vector origin, vector mins, vector maxs);

bool CheckGroundSpawnPoint(vector origin, vector ent_mins, vector ent_maxs, float height, float gravity);

entityref CreateGroundMonster(vector origin, vector angles, vector mins, vector maxs, const entity_type &type, int32_t height);

bool FindSpawnPoint(vector startpoint, vector mins, vector maxs, vector &spawnpoint, float max_move_up);

void SpawnGrow_Spawn(vector startpos, int32_t size);

void ThrowWidowGibReal(entity &self, stringlit gibname, int32_t damage, gib_type type, vector startpos, bool sized, sound_index hitsound, bool fade);

void Widowlegs_Spawn(vector startpos, vector cangles);
#endif
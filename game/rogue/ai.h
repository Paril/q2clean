#pragma once

#include "config.h"

#ifdef SINGLE_PLAYER
#include "game/entity_types.h"
#include "lib/math/vector.h"

#ifdef ROGUE_AI
#include "game/savables.h"
#include "game/spawn.h"

DECLARE_ENTITY(BAD_AREA);

bool blocked_checkshot(entity &self, float shot_chance);

bool blocked_checkplat(entity &self, float dist);

bool blocked_checkjump(entity &self, float dist, float max_down, float max_up);

bool has_valid_enemy(entity &self);

void hintpath_stop(entity &self);

bool monsterlost_checkhint(entity &self);

void InitHintPaths();

bool inback(entity &self, entity &other);

entity &SpawnBadArea(vector cmins, vector cmaxs, gtime lifespan_frames, entityref cowner);

entityref CheckForBadArea(entity &ent);

void PredictAim(entityref target, vector start, float bolt_speed, bool eye_height, float offset, vector *aimdir, vector *aimpoint);

bool below(entity &self, entity &other);

void monster_done_dodge(entity &self);

void M_MonsterDodge(entity &self, entity &attacker, float eta, trace &tr);

DECLARE_SAVABLE(M_MonsterDodge);

void monster_duck_down(entity &self);

void monster_duck_hold(entity &self);

void monster_duck_up(entity &self);

DECLARE_SAVABLE(monster_duck_up);

void monster_jump_start(entity &self);

bool monster_jump_finished(entity &self);

bool IsBadAhead(entity &self, entity &bad, vector move);
#endif

#ifdef GROUND_ZERO
bool MarkTeslaArea(entity &tesla);

void TargetTesla(entity &self, entity &tesla);

entityref PickCoopTarget(entity &self);

uint8_t CountPlayers();
#endif

#endif
#pragma once

#include "config.h"
#include "entity_types.h"

#ifdef SINGLE_PLAYER
#include "game_types.h"

bool FindTarget(entity &self);

extern bool		enemy_vis;
extern float	enemy_range;
extern float	enemy_yaw;

// melee range, will become hostile even if back is turned
constexpr float RANGE_MELEE	= 80.f;
// visibility and infront, or visibility and show hostile
constexpr float RANGE_NEAR = 500.f;
// infront and show hostile
constexpr float RANGE_MID = 1000.f;
// otherwise, only triggered by damage

/*
=================
AI_SetSightClient

Called once each frame to set level.sight_client to the
player to be checked for in findtarget.

If all clients are either dead or in notarget, sight_client
will be null.

In coop games, sight_client will cycle between the clients.
=================
*/
void AI_SetSightClient();

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move(entity &self, float dist);

/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
void ai_stand(entity &self, float dist);

/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void ai_walk(entity &self, float dist);

/*
=============
ai_charge

Turns towards target and advances
Use this call with a distnace of 0 to replace ai_face
==============
*/
void ai_charge(entity &self, float dist);

/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void ai_turn(entity &self, float dist);

void AttackFinished(entity &self, gtimef time);

void HuntTarget(entity &self);

void FoundTarget(entity &self);

/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
bool FindTarget(entity &self);

bool M_CheckAttack(entity &self);

DECLARE_SAVABLE(M_CheckAttack);

/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
bool ai_checkattack(entity &self, float dist);

/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run(entity &self, float dist);

#ifdef ROGUE_AI
// this determines how long to wait after a duck to duck again.  this needs to be longer than
// the time after the monster_duck_up in all of the animation sequences
constexpr gtime DUCK_INTERVAL	= 500ms;
#endif

#ifdef GROUND_ZERO
// this is for the count of monsters
inline int SELF_SLOTS_LEFT(entity &self)
{
	return (self.monsterinfo.monster_slots - self.monsterinfo.monster_used);
};
#endif

#endif
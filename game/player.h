#pragma once

#include "config.h"
#include "lib/string.h"
#include "lib/math/vector.h"
#include "lib/protocol.h"
#include "game/entity_types.h"
#include "game/spawn.h"

void player_die(entity &self, entity &inflictor, entity &attacker, int32_t damage, vector);

void InitClientPersistant(entity &ent);

float PlayersRangeFromSpot(entity &spot);

entityref SelectRandomDeathmatchSpawnPoint();

entityref SelectFarthestDeathmatchSpawnPoint();

void InitBodyQue();

void PutClientInServer(entity &self);

void ClientUserinfoChanged(entity &ent, string userinfo);

bool ClientConnect(entity &ent, string &userinfo);

void ClientDisconnect(entity &ent);

void respawn(entity &self);

void ClientBeginServerFrame(entity &ent);

void ClientBegin(entity &ent);

void ClientThink(entity &ent, const usercmd &ucmd);

DECLARE_ENTITY(INFO_PLAYER_START);

DECLARE_ENTITY(INFO_PLAYER_DEATHMATCH);

DECLARE_ENTITY(INFO_PLAYER_INTERMISSION);

#ifdef SINGLE_PLAYER
DECLARE_ENTITY(INFO_PLAYER_COOP);

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData();
#endif
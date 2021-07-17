#pragma once

#include "../lib/types.h"

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

#ifdef SINGLE_PLAYER
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
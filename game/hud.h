#pragma once

#include "entity_types.h"

/*
==================
DeathmatchScoreboardMessage
==================
*/
void DeathmatchScoreboardMessage(entity &ent, entityref killer, bool reliable = true);

void MoveClientToIntermission(entity &ent);

void BeginIntermission(entity &targ);

/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f(entity &ent);

/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f(entity &ent);

/*
===============
G_SetStats
===============
*/
void G_SetStats(entity &ent);

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats(entity &ent);

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats(entity &ent);
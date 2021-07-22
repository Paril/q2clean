#pragma once

import "config.h";
import "entity_types.h";

#ifdef SINGLE_PLAYER

/*
=============
M_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.
=============
*/
bool M_CheckBottom(entity &ent);

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done and false is returned
=============
*/
bool SV_movestep(entity &ent, vector move, bool relink);

/*
===============
M_ChangeYaw

===============
*/
void M_ChangeYaw(entity &ent);

/*
================
SV_NewChaseDir

================
*/
void SV_NewChaseDir(entity &actor, entityref enemy, float dist);

/*
======================
M_MoveToGoal
======================
*/
void M_MoveToGoal(entity &ent, float dist);

/*
===============
M_walkmove
===============
*/
bool M_walkmove(entity &ent, float yaw, float dist);

#endif
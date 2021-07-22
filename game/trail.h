#pragma once

import "config.h";

#ifdef SINGLE_PLAYER
import "entity_types.h";
import "game.h";
import "lib/math/vector.h";

/*
==============================================================================

PLAYER TRAIL

==============================================================================

This is a circular list containing the a list of points of where
the player has been recently.  It is used by monsters for pursuit.

*/

entityref PlayerTrail_LastSpot();

entityref PlayerTrail_PickNext(entity &self);

entityref PlayerTrail_PickFirst(entity &self);

void PlayerTrail_New(vector spot);

void PlayerTrail_Add(vector spot);

void PlayerTrail_Init();

#endif
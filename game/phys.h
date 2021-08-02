#pragma once

#include "lib/types.h"
#include "game/entity_types.h"

// This constant is used in Q2 for movement
constexpr int32_t STEPSIZE = 18;

// The max number of planes to clip moves against
constexpr size_t MAX_CLIP_PLANES = 5;

void SV_AddGravity(entity &ent);

void G_RunEntity(entity &ent);

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(entity &e1, const trace &tr);
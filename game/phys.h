#pragma once

#include "../lib/types.h"

// This constant is used in Q2 for movement
constexpr int32_t STEPSIZE = 18;

// The max number of planes to clip moves against
constexpr size_t MAX_CLIP_PLANES = 5;

void SV_AddGravity(entity &ent);

void G_RunEntity(entity &ent);

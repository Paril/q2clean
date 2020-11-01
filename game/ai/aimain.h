#pragma once

#include "../../lib/types.h"

void AI_SetUpMoveWander(entity &ent);

void AI_NewMap();

void AI_Init();

void AI_ResetWeights(entity &ent);

void AI_ResetNavigation(entity &ent);

bool AI_BotRoamForLRGoal(entity &self, node_id current_node);

void AI_PickLongRangeGoal(entity &self);

void AI_PickShortRangeGoal(entity &self);

void AI_Think(entity &self);
#pragma once

#include "ai.h"

node_id AI_FindClosestReachableNode(vector origin, entityref passent, int32_t range, node_flags flagsmask);

float AI_FindCost(node_id from, node_id to, ai_link_type movetypes);

void AI_SetGoal(entity &self, node_id goal_node);

bool AI_FollowPath(entity &self);

void AI_InitNavigationData();

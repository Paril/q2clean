#pragma once

#include "../../lib/types.h"
#include "ai.h"

node_id AI_findNodeInRadius(node_id from, vector org, float rad, bool ignoreHeight);

float AI_FindLinkDistance(node_id n1, node_id n2);

bool AI_PlinkExists(node_id n1, node_id n2);

bool AI_AddLink(node_id n1, node_id n2, ai_link_type linkType);

ai_link_type AI_PlinkMoveType(node_id n1, node_id n2);

ai_link_type AI_GravityBoxToLink(node_id n1, node_id n2);

ai_link_type AI_FindLinkType(node_id n1, node_id n2);

uint32_t AI_LinkCloseNodes_JumpPass( node_id start );

uint32_t AI_LinkCloseNodes();

#pragma once

#include "../../lib/types.h"

enum ai_direction_t : uint8_t
{
	BOT_MOVE_LEFT,
	BOT_MOVE_RIGHT,
	BOT_MOVE_FORWARD,
	BOT_MOVE_BACK
};

bool AI_IsStep(entity &ent);

bool AI_CanMove(entity &self, ai_direction_t direction);

bool AI_IsLadder(vector origin, vector v_angle, vector mins, vector maxs, entityref passent);

bool AI_SpecialMove(entity &self, usercmd &ucmd);

void AI_ChangeAngle(entity &ent);

bool AI_MoveToGoalEntity(entity &self, usercmd &ucmd);

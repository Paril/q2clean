#pragma once

#include "../../lib/types.h"
#include "../items.h"

enum ai_aimstyle : uint8_t
{
	AI_AIMSTYLE_INSTANTHIT,
	AI_AIMSTYLE_PREDICTION,
	AI_AIMSTYLE_PREDICTION_EXPLOSIVE,
	AI_AIMSTYLE_DROP,
	
	AIWEAP_AIM_TYPES
};

enum ai_range : uint8_t
{
	AIWEAP_LONG_RANGE,
	AIWEAP_MEDIUM_RANGE,
	AIWEAP_SHORT_RANGE,
	AIWEAP_MELEE_RANGE,
	
	AIWEAP_RANGES
};

struct ai_weapon
{
	ai_aimstyle	aimType;

	array<float, AIWEAP_RANGES>	RangeWeight;

	gitem_id	weaponItem;
	gitem_id	ammoItem;
};

extern ai_weapon AIWeapons[ITEM_TOTAL];

void AI_InitAIWeapons ();

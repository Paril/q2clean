#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/entity.h"
#include "aiweapons.h"

ai_weapon AIWeapons[ITEM_TOTAL];

//==========================================
// AI_InitAIWeapons
// 
// AIWeapons are the way the AI uses to analize
// weapon types, for choosing and fire them
//==========================================
void AI_InitAIWeapons ()
{
	AIWeapons[ITEM_BLASTER] = {
		AI_AIMSTYLE_PREDICTION,
		{ 0.05f, 0.05f, 0.1f, 0.2f },
		ITEM_BLASTER
	};

	AIWeapons[ITEM_SHOTGUN] = {
		AI_AIMSTYLE_INSTANTHIT,
		{ 0.1f, 0.1f, 0.2f, 0.3f },
		ITEM_SHOTGUN,
		ITEM_SHELLS
	};
	
	AIWeapons[ITEM_SUPER_SHOTGUN] = { 
		AI_AIMSTYLE_INSTANTHIT,
		{ 0.2f, 0.2f, 0.6f, 0.7f },
		ITEM_SUPER_SHOTGUN,
		ITEM_SHELLS
	};

	AIWeapons[ITEM_MACHINEGUN] = {
		AI_AIMSTYLE_INSTANTHIT,
		{ 0.3f, 0.3f, 0.3f, 0.4f },
		ITEM_MACHINEGUN,
		ITEM_BULLETS
	};

	AIWeapons[ITEM_CHAINGUN] = {
		AI_AIMSTYLE_INSTANTHIT,
		{ 0.4f, 0.6f, 0.7f, 0.7f },
		ITEM_CHAINGUN,
		ITEM_BULLETS
	};

	AIWeapons[ITEM_GRENADES] = { 
		AI_AIMSTYLE_DROP,
		{ 0.0, 0.0, 0.2f, 0.2f },
		ITEM_GRENADES,
		ITEM_GRENADES
	};

	AIWeapons[ITEM_GRENADE_LAUNCHER] = {
		AI_AIMSTYLE_DROP,
		{ 0.0, 0.0, 0.3f, 0.5f },
		ITEM_GRENADE_LAUNCHER,
		ITEM_GRENADES
	};

	AIWeapons[ITEM_ROCKET_LAUNCHER] = {
		AI_AIMSTYLE_PREDICTION_EXPLOSIVE,
		{ 0.2f, 0.7f, 0.9f, 0.6f },
		ITEM_ROCKET_LAUNCHER,
		ITEM_ROCKETS
	};

	AIWeapons[ITEM_HYPERBLASTER] = {
		AI_AIMSTYLE_PREDICTION,
		{ 0.1f, 0.1f, 0.2f, 0.3f },
		ITEM_HYPERBLASTER,
		ITEM_CELLS
	};

	AIWeapons[ITEM_RAILGUN] = {
		AI_AIMSTYLE_INSTANTHIT,
		{ 0.9f, 0.6f, 0.4f, 0.3f },
		ITEM_RAILGUN,
		ITEM_SLUGS
	};

	AIWeapons[ITEM_BFG] = {
		AI_AIMSTYLE_PREDICTION_EXPLOSIVE,
		{ 0.3f, 0.6f, 0.7f, 0.0 },
		ITEM_BFG,
		ITEM_CELLS
	};
}


#endif
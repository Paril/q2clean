module;

#include "lib/types.h"

export module weaponry.handgrenade;

export constexpr float GRENADE_TIMER	= 3.0f;
export constexpr int GRENADE_MINSPEED	= 400;
export constexpr int GRENADE_MAXSPEED	= 800;

export void weapon_grenade_fire(entity &ent, bool held);

export void Weapon_Grenade(entity &ent);
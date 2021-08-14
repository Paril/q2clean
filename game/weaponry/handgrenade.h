#include "lib/types.h"
#include "game/entity_types.h"

constexpr gtimef GRENADE_TIMER		= 3s;
constexpr int32_t GRENADE_MINSPEED	= 400;
constexpr int32_t GRENADE_MAXSPEED	= 800;

void weapon_grenade_fire(entity &ent, bool held);

void Weapon_Grenade(entity &ent);
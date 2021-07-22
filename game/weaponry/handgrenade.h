#include "lib/types.h"
#include "game/entity_types.h"

constexpr float GRENADE_TIMER	= 3.0f;
constexpr int32_t GRENADE_MINSPEED	= 400;
constexpr int32_t GRENADE_MAXSPEED	= 800;

void weapon_grenade_fire(entity &ent, bool held);

void Weapon_Grenade(entity &ent);
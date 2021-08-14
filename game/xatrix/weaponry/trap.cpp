#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/weaponry/handgrenade.h"
#include "game/xatrix/ballistics/trap.h"
#include "trap.h"

void weapon_trap_fire(entity &ent, bool held)
{
	int32_t	damage = 125;
	float radius = damage + 40.f;

	if (is_quad)
		damage *= damage_multiplier;

	vector offset = { 8, 8, ent.viewheight - 8.f };
	vector forward, right;
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
	vector start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	gtime timer = ent.client->grenade_framenum - level.framenum;
	int32_t speed = (int32_t) (GRENADE_MINSPEED + (GRENADE_TIMER - timer).count() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.count()));
	fire_trap(ent, start, forward, damage, speed, radius, held);

	ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->grenade_framenum = level.framenum + 1s;
}

void Weapon_Trap(entity &ent)
{
	Throw_Generic(ent, 0, 15, 48, 5, "weapons/trapcock.wav", 11, "weapons/traploop.wav", 12, G_FrameIsOneOf<29, 34, 39, 48>, weapon_trap_fire, false);
}
#endif
#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/player_frames.h"
#include "tesla.h"
#include "game/weaponry/handgrenade.h"
#include "game/rogue/ballistics/tesla.h"

static void weapon_tesla_fire(entity &ent, bool)
{
	int32_t	damage = 125;

	if (is_quad)
		damage *= damage_multiplier;		// PGM

	vector forward, right, up;
	AngleVectors (ent.client->v_angle, &forward, &right, &up);

	vector offset { 0, -4, ent.viewheight - 22.f };
	vector start = P_ProjectSource(ent, ent.origin, offset, forward, right, up);

	float timer = (ent.client->grenade_framenum - level.framenum) * FRAMETIME;
	int32_t speed = (int32_t)(GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	if (speed > GRENADE_MAXSPEED)
		speed = GRENADE_MAXSPEED;

	fire_tesla (ent, start, forward, damage_multiplier, speed);

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->grenade_framenum = level.framenum + BASE_FRAMERATE;

	if (ent.deadflag || ent.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.health <= 0)
		return;

	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.client->anim_priority = ANIM_ATTACK;
		ent.frame = FRAME_crattak1-1;
		ent.client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent.client->anim_priority = ANIM_REVERSE;
		ent.frame = FRAME_wave08;
		ent.client->anim_end = FRAME_wave01;
	}
}

void Weapon_Tesla(entity &ent)
{
	if ((ent.client->ps.gunframe > 1) && (ent.client->ps.gunframe < 9))
		ent.client->ps.gunindex = gi.modelindex("models/weapons/v_tesla2/tris.md2");
	else
		ent.client->ps.gunindex = gi.modelindex("models/weapons/v_tesla/tris.md2");

	Throw_Generic(ent, 0, 8, 32, -1, nullptr, 1, nullptr, 2, G_FrameIsOneOf<21>, weapon_tesla_fire, false);
}
#endif
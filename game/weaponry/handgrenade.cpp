#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/grenade.h"
#include "game/player_frames.h"
#include "handgrenade.h"

void weapon_grenade_fire(entity &ent, bool held)
{
	vector	offset;
	vector	forward, right;
	vector	start;
	const int32_t damage = 125 * damage_multiplier;
	int32_t	speed;
	const float radius = (damage / damage_multiplier) + 40.f;

	offset = { 8, 8, ent.viewheight - 8.f };
	AngleVectors(ent.client.v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	gtimef timer = (ent.client.grenade_time - level.time);
	speed = (int32_t) (GRENADE_MINSPEED + (GRENADE_TIMER - timer).count() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.count()));
	fire_grenade2(ent, start, forward, damage, speed, timer, radius, held);

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index]--;

	ent.client.grenade_time = level.time + 1s;

	if (ent.deadflag || ent.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.health <= 0)
		return;

	if (ent.client.ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.client.anim_priority = ANIM_ATTACK;
		ent.frame = FRAME_crattak1 - 1;
		ent.client.anim_end = FRAME_crattak3;
	}
	else
	{
		ent.client.anim_priority = ANIM_REVERSE;
		ent.frame = FRAME_wave08;
		ent.client.anim_end = FRAME_wave01;
	}
}

void Weapon_Grenade(entity &ent)
{
	Throw_Generic(ent, 0, 15, 48, 5, "weapons/hgrena1b.wav", 11, "weapons/hgrenc1b.wav", 12, G_FrameIsOneOf<29, 34, 39, 48>, weapon_grenade_fire, true);
}
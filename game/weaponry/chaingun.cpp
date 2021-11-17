#include "config.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/ballistics/bullet.h"
#include "game/player_frames.h"
#include "game/entity.h"
#include "game/game.h"
#include "chaingun.h"

constexpr means_of_death MOD_CHAINGUN { .other_kill_fmt = "{0} was cut in half by {3}'s chaingun.\n" };

static void Chaingun_Fire(entity &ent)
{
	vector	start = vec3_origin;
	vector	forward, right, up;
	float	r, u;
	vector	offset;
	const int32_t kick = 2 * damage_multiplier;
#ifdef SINGLE_PLAYER
	const int32_t damage = (!deathmatch ? 8 : 6) * damage_multiplier;
#else
	const int32_t damage = 6 * damage_multiplier;
#endif

	if (ent.client.ps.gunframe == 5)
		gi.sound(ent, gi.soundindex("weapons/chngnu1a.wav"), ATTN_IDLE);

	if ((ent.client.ps.gunframe == 14) && !(ent.client.buttons & BUTTON_ATTACK))
	{
		ent.client.ps.gunframe = 32;
		ent.client.weapon_sound = SOUND_NONE;
		return;
	}
	else if ((ent.client.ps.gunframe == 21) && (ent.client.buttons & BUTTON_ATTACK)
		&& ent.client.pers.inventory[ent.client.ammo_index])
		ent.client.ps.gunframe = 15;
	else
		ent.client.ps.gunframe++;

	if (ent.client.ps.gunframe == 22)
	{
		ent.client.weapon_sound = SOUND_NONE;
		gi.sound(ent, gi.soundindex("weapons/chngnd1a.wav"), ATTN_IDLE);
	}
	else
		ent.client.weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	ent.client.anim_priority = ANIM_ATTACK;
	if (ent.client.ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.frame = FRAME_crattak1 - (ent.client.ps.gunframe & 1);
		ent.client.anim_end = FRAME_crattak9;
	}
	else
	{
		ent.frame = FRAME_attack1 - (ent.client.ps.gunframe & 1);
		ent.client.anim_end = FRAME_attack8;
	}

	int32_t shots;

	if (ent.client.ps.gunframe <= 9)
		shots = 1;
	else if (ent.client.ps.gunframe <= 14)
	{
		if (ent.client.buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent.client.pers.inventory[ent.client.ammo_index] < shots)
		shots = ent.client.pers.inventory[ent.client.ammo_index];

	if (!shots)
	{
		if (level.time >= ent.pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"));
			ent.pain_debounce_time = level.time + 1s;
		}
		NoAmmoWeaponChange(ent);
		return;
	}

	ent.client.kick_origin = crandomv({ 0.35f, 0.35f, 0.35f });
	ent.client.kick_angles = crandomv({ 0.7f, 0.7f, 0.7f });

	for (int32_t i = 0; i < shots; i++)
	{
		// get start / end positions
		AngleVectors(ent.client.v_angle, &forward, &right, &up);
		r = random(3.f, 11.f);
		u = crandom(4.f);
		offset = { 0, r, u + ent.viewheight - 8 };
		start = P_ProjectSource(ent, ent.origin, offset, forward, right);

		fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, (muzzleflash) (MZ_CHAINGUN1 + shots - 1) | is_silenced).multicast(ent.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index] -= shots;
}

void Weapon_Chaingun(entity &ent)
{
	Weapon_Generic(ent, 4, 31, 61, 64, G_FrameIsOneOf<38, 43, 51, 61>, G_FrameIsBetween<5, 21>, Chaingun_Fire);
}
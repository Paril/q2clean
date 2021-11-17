#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/player_frames.h"
#include "game/ballistics/bullet.h"
#include "machinegun.h"

constexpr means_of_death MOD_MACHINEGUN { .other_kill_fmt = "{0} was machinegunned by {3}.\n" };

static void Machinegun_Fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	angles;
	const int32_t damage = 8 * damage_multiplier;
	const int32_t kick = 2 * damage_multiplier;
	vector	offset;

	if (!(ent.client.buttons & BUTTON_ATTACK)) {
		ent.client.machinegun_shots = 0;
		ent.client.ps.gunframe++;
		return;
	}

	if (ent.client.ps.gunframe == 5)
		ent.client.ps.gunframe = 4;
	else
		ent.client.ps.gunframe = 5;

	if (ent.client.pers.inventory[ent.client.ammo_index] < 1)
	{
		ent.client.ps.gunframe = 6;
		if (level.time >= ent.pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"));
			ent.pain_debounce_time = level.time + 1s;
		}
		NoAmmoWeaponChange(ent);
		return;
	}
	
	ent.client.kick_origin = crandomv({ 0.35f, 0.35f, 0.35f });

	ent.client.kick_angles[1] = crandom(0.7f);
	ent.client.kick_angles[2] = crandom(0.7f);

	ent.client.kick_angles[0] = ent.client.machinegun_shots * -1.5f;
#ifdef SINGLE_PLAYER

	// raise the gun as it is firing
	if (!deathmatch)
	{
		ent.client.machinegun_shots++;
		if (ent.client.machinegun_shots > 9)
			ent.client.machinegun_shots = 9;
	}
#endif

	// get start / end positions
	angles = ent.client.v_angle + ent.client.kick_angles;
	AngleVectors(angles, &forward, &right, nullptr);
	offset = { 0, 8, ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.origin, offset, forward, right);

	fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.ConstructMessage(svc_muzzleflash, ent, MZ_MACHINEGUN | is_silenced).multicast(ent.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index]--;

	ent.client.anim_priority = ANIM_ATTACK;
	if (ent.client.ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.frame = FRAME_crattak1 - (int) random(0.25f, 1.25f);
		ent.client.anim_end = FRAME_crattak9;
	}
	else
	{
		ent.frame = FRAME_attack1 - (int) random(0.25f, 1.25f);
		ent.client.anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun(entity &ent)
{
	Weapon_Generic(ent, 3, 5, 45, 49, G_FrameIsOneOf<23, 45>, G_FrameIsOneOf<4, 5>, Machinegun_Fire);
}
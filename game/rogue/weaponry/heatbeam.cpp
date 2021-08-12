#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/rogue/ballistics/heatbeam.h"
#include "game/player_frames.h"
#include "heatbeam.h"

static void Heatbeam_Fire(entity &ent)
{
	if (ent.client->pers.inventory[ent.client->ammo_index] < ent.client->pers.weapon->quantity)
	{
		ent.client->ps.gunframe++;
		return;
	}

	int32_t damage = 15;
#ifdef SINGLE_PLAYER
	int32_t kick;

	if (!deathmatch)
		kick = 30;
	else
		kick = 75;
#else
	int32_t kick = 75;
#endif

	ent.client->ps.gunframe++;
	ent.client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent.client->kick_origin = ent.client->kick_angles = vec3_origin;

	// get start / end positions
	vector forward, right, up;
	AngleVectors (ent.client->v_angle, &forward, &right, &up);

// This offset is the "view" offset for the beam start (used by trace)
	
	vector offset { 7, 2, ent.viewheight - 3.f };
	vector start = P_ProjectSource (ent, ent.origin, offset, forward, right);

	fire_heatbeam (ent, start, forward, damage, kick, false);

	// send muzzle flash
	gi.ConstructMessage(svc_muzzleflash, ent, MZ_HEATBEAM | is_silenced).multicast (ent.origin, MULTICAST_PVS);
#if defined(SINGLE_PLAYER)

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= ent.client->pers.weapon->quantity;

	ent.client->anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.frame = FRAME_crattak1 - 1;
		ent.client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent.frame = FRAME_attack1 - 1;
		ent.client->anim_end = FRAME_attack8;
	}
}

void Weapon_Heatbeam(entity &ent)
{
	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		ent.client->weapon_sound = gi.soundindex("weapons/bfg__l1a.wav");

		if ((ent.client->pers.inventory[ent.client->ammo_index] >= ent.client->pers.weapon->quantity) && (ent.client->buttons & BUTTON_ATTACK))
		{
			if (ent.client->ps.gunframe >= 13)
			{
				ent.client->ps.gunframe = 9;
				ent.client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");
			}
			else
				ent.client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");
		}
		else
		{
			ent.client->ps.gunframe = 13;
			ent.client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer/tris.md2");
		}
	}
	else
	{
		ent.client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer/tris.md2");
		ent.client->weapon_sound = SOUND_NONE;
	}

	Weapon_Generic (ent, 8, 12, 39, 44, G_FrameIsOneOf<35>, G_FrameIsBetween<9, 12>, Heatbeam_Fire);
}
#endif
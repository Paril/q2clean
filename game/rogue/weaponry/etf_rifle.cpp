#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/player_frames.h"
#include "etf_rifle.h"
#include "game/rogue/ballistics/flechette.h"

static void weapon_etf_rifle_fire(entity &ent)
{
	// PGM - adjusted to use the quantity entry in the weapon structure.
	if (ent.client->pers.inventory[ent.client->ammo_index] < ent.client->pers.weapon->quantity)
	{
		ent.client->kick_origin = ent.client->kick_angles = vec3_origin;
		ent.client->ps.gunframe = 8;

		if (level.framenum >= ent.pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent.pain_debounce_framenum = level.framenum + (1 * BASE_FRAMERATE);
		}

		NoAmmoWeaponChange (ent);
		return;
	}

	int	damage = 10;
	int	kick = 3;

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent.client->kick_origin = randomv({ -0.85f, -0.85f, -0.85f }, { 0.85f, 0.85f, 0.85f });
	ent.client->kick_angles = randomv({ -0.85f, -0.85f, -0.85f }, { 0.85f, 0.85f, 0.85f });

	// get start / end positions
	vector forward, right, up;
	AngleVectors (ent.client->v_angle, &forward, &right, &up);

	vector offset;
	if (ent.client->ps.gunframe == 6)			// right barrel
		offset = { 15, 8, -8 };
	else										// left barrel
		offset = { 15, 6, -8 };
	
	vector v = ent.s.origin;
	v[2] += ent.viewheight;
	vector start = P_ProjectSource(ent, v, offset, forward, right, up);
	fire_flechette (ent, start, forward, damage, 750, kick);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent.s.number);
	gi.WriteByte (MZ_ETF_RIFLE);
	gi.multicast (ent.s.origin, MULTICAST_PVS);
#if defined(SINGLE_PLAYER)

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	ent.client->ps.gunframe++;
	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= ent.client->pers.weapon->quantity;

	ent.client->anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crattak1 - 1;
		ent.client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent.s.frame = FRAME_attack1 - 1;
		ent.client->anim_end = FRAME_attack8;
	}
}

void Weapon_ETF_Rifle(entity &ent)
{
	if (ent.client->weaponstate == WEAPON_FIRING)
		if (ent.client->pers.inventory[ent.client->ammo_index] <= 0)
			ent.client->ps.gunframe = 8;

	Weapon_Generic(ent, 4, 7, 37, 41, G_IsAnyFrame<18, 28>, G_IsAnyFrame<6, 7>, weapon_etf_rifle_fire);

	if (ent.client->ps.gunframe == 8 && (ent.client->buttons & BUTTON_ATTACK))
		ent.client->ps.gunframe = 6;
}
#endif
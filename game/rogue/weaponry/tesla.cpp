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

// FIXME: move to weaponry and use for hg and trap
static void Throw_Generic(entity &ent, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_THROW_SOUND, int32_t FRAME_THROW_HOLD, int32_t FRAME_THROW_FIRE, std::initializer_list<int32_t> pause_frames, void (*fire)(entity &, bool))
{
	const int32_t FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;

	if ((ent.client->newweapon) && (ent.client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent.client->weaponstate == WEAPON_ACTIVATING)
	{
		ent.client->weaponstate = WEAPON_READY;
		ent.client->ps.gunframe = FRAME_IDLE_FIRST;
		return;
	}

	if (ent.client->weaponstate == WEAPON_READY)
	{
		if ((ent.client->latched_buttons | ent.client->buttons) & BUTTON_ATTACK)
		{
			ent.client->latched_buttons &= ~BUTTON_ATTACK;

			if (ent.client->pers.inventory[ent.client->ammo_index])
			{
				ent.client->ps.gunframe = 1;
				ent.client->weaponstate = WEAPON_FIRING;
				ent.client->grenade_framenum = 0;
				return;
			}
			else if (level.framenum >= ent.pain_debounce_framenum)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent.pain_debounce_framenum = level.framenum + (1 * BASE_FRAMERATE);
			}

			NoAmmoWeaponChange (ent);
			return;
		}

		if (ent.client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent.client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		for (auto &frame : pause_frames)
		{
			if (ent.client->ps.gunframe == frame)
			{
				if (Q_rand() & 15)
					return;

				break;
			}
		}

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		if (ent.client->ps.gunframe == FRAME_THROW_SOUND)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent.client->ps.gunframe == FRAME_THROW_HOLD)
		{
			if (!ent.client->grenade_framenum)
				ent.client->grenade_framenum = level.framenum + (int)((GRENADE_TIMER + 0.2) * BASE_FRAMERATE);

			if (ent.client->buttons & BUTTON_ATTACK)
				return;

			if (ent.client->grenade_blew_up)
			{
				if (level.framenum >= ent.client->grenade_framenum)
				{
					ent.client->ps.gunframe = FRAME_FIRE_LAST;
					ent.client->grenade_blew_up = false;
				}
				else
					return;
			}
		}

		if (ent.client->ps.gunframe == FRAME_THROW_FIRE)
		{
			ent.client->weapon_sound = SOUND_NONE;
			fire(ent, true);
		}

		if ((ent.client->ps.gunframe == FRAME_FIRE_LAST) && (level.framenum < ent.client->grenade_framenum))
			return;

		ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == FRAME_IDLE_FIRST)
		{
			ent.client->grenade_framenum = 0;
			ent.client->weaponstate = WEAPON_READY;
		}
	}
}

static void weapon_tesla_fire(entity &ent, bool)
{
	int32_t	damage = 125;

	if (is_quad)
		damage *= damage_multiplier;		// PGM

	vector forward, right, up;
	AngleVectors (ent.client->v_angle, &forward, &right, &up);

	vector offset { 0, -4, ent.viewheight - 22.f };
	vector start = P_ProjectSource(ent, ent.s.origin, offset, forward, right, up);

	float timer = ((gtimediff) ent.client->grenade_framenum - (gtimediff) level.framenum) * FRAMETIME;
	int32_t speed = (int32_t)(GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	if (speed > GRENADE_MAXSPEED)
		speed = GRENADE_MAXSPEED;

	fire_tesla (ent, start, forward, damage_multiplier, speed);

	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->grenade_framenum = level.framenum + BASE_FRAMERATE;

	if (ent.deadflag || ent.s.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.health <= 0)
		return;

	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.client->anim_priority = ANIM_ATTACK;
		ent.s.frame = FRAME_crattak1-1;
		ent.client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent.client->anim_priority = ANIM_REVERSE;
		ent.s.frame = FRAME_wave08;
		ent.client->anim_end = FRAME_wave01;
	}
}

void Weapon_Tesla(entity &ent)
{
	if ((ent.client->ps.gunframe > 1) && (ent.client->ps.gunframe < 9))
		ent.client->ps.gunindex = gi.modelindex("models/weapons/v_tesla2/tris.md2");
	else
		ent.client->ps.gunindex = gi.modelindex("models/weapons/v_tesla/tris.md2");

	Throw_Generic(ent, 8, 32, 99, 1, 2, { 21 }, weapon_tesla_fire);
}
#endif
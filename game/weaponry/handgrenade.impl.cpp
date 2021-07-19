module;

#include "lib/types.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/gweapon.h"
#include "game/m_player.h"

module weaponry.handgrenade;

import weaponry;
import player.view;
import math.vector;
import math.random;

void weapon_grenade_fire(entity &ent, bool held)
{
	vector	offset;
	vector	forward, right;
	vector	start;
	int32_t	damage = 125;
	float	timer;
	int32_t	speed;
	float	radius;

	radius = damage + 40.f;
	if (is_quad)
		damage *= damage_multiplier;

	offset = { 8, 8, ent.viewheight - 8.f };
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	timer = (ent.client->grenade_framenum - level.framenum) * FRAMETIME;
	speed = (int32_t) (GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	fire_grenade2(ent, start, forward, damage, speed, timer, radius, held);

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->grenade_framenum = (gtime) (level.framenum + 1.0f * BASE_FRAMERATE);

	if (ent.deadflag || ent.s.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.health <= 0)
		return;

	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.client->anim_priority = ANIM_ATTACK;
		ent.s.frame = FRAME_crattak1 - 1;
		ent.client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent.client->anim_priority = ANIM_REVERSE;
		ent.s.frame = FRAME_wave08;
		ent.client->anim_end = FRAME_wave01;
	}
}

void Weapon_Grenade(entity &ent)
{
	if ((ent.client->newweapon) && (ent.client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent);
		return;
	}

	if (ent.client->weaponstate == WEAPON_ACTIVATING)
	{
		ent.client->weaponstate = WEAPON_READY;
		ent.client->ps.gunframe = 16;
		return;
	}

	if (ent.client->weaponstate == WEAPON_READY)
	{
		if (((ent.client->latched_buttons | ent.client->buttons) & BUTTON_ATTACK))
		{
			ent.client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent.client->pers.inventory[ent.client->ammo_index])
			{
				ent.client->ps.gunframe = 1;
				ent.client->weaponstate = WEAPON_FIRING;
				ent.client->grenade_framenum = 0;
			}
			else
			{
				if (level.framenum >= ent.pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
				}
				NoAmmoWeaponChange(ent);
			}
			return;
		}

		if ((ent.client->ps.gunframe == 29) || (ent.client->ps.gunframe == 34) || (ent.client->ps.gunframe == 39) || (ent.client->ps.gunframe == 48))
			if (Q_rand() & 15)
				return;

		if (++ent.client->ps.gunframe > 48)
			ent.client->ps.gunframe = 16;
		return;
	}

	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		if (ent.client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent.client->ps.gunframe == 11)
		{
			if (!ent.client->grenade_framenum) {
				ent.client->grenade_framenum = (gtime) (level.framenum + (GRENADE_TIMER + 0.2f) * BASE_FRAMERATE);
				ent.client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent.client->grenade_blew_up && level.framenum >= ent.client->grenade_framenum)
			{
				ent.client->weapon_sound = SOUND_NONE;
				weapon_grenade_fire(ent, true);
				ent.client->grenade_blew_up = true;
			}

			if (ent.client->buttons & BUTTON_ATTACK)
				return;

			if (ent.client->grenade_blew_up)
			{
				if (level.framenum >= ent.client->grenade_framenum)
				{
					ent.client->ps.gunframe = 15;
					ent.client->grenade_blew_up = false;
				}
				else
					return;
			}
		}

		if (ent.client->ps.gunframe == 12)
		{
			ent.client->weapon_sound = SOUND_NONE;
			weapon_grenade_fire(ent, false);
		}

		if ((ent.client->ps.gunframe == 15) && (level.framenum < ent.client->grenade_framenum))
			return;

		ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == 16)
		{
			ent.client->grenade_framenum = 0;
			ent.client->weaponstate = WEAPON_READY;
		}
	}
}
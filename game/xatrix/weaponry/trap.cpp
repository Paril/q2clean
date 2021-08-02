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
	vector start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	float timer = (ent.client->trap_framenum - level.framenum) * FRAMETIME;
	int32_t speed = (int32_t) (GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	fire_trap(ent, start, forward, damage, speed, timer, radius, held);

	ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->trap_framenum = (gtime) (level.framenum + 1.0f * BASE_FRAMERATE);
}

void Weapon_Trap(entity &ent)
{
	if (ent.client->newweapon && (ent.client->weaponstate == WEAPON_READY))
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
		if ((ent.client->latched_buttons | ent.client->buttons) & BUTTON_ATTACK)
		{
			ent.client->latched_buttons &= ~BUTTON_ATTACK;

			if (ent.client->pers.inventory[ent.client->ammo_index])
			{
				ent.client->ps.gunframe = 1;
				ent.client->weaponstate = WEAPON_FIRING;
				ent.client->trap_framenum = 0;
				return;
			}

			if (level.framenum >= ent.pain_debounce_framenum)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
			}

			NoAmmoWeaponChange(ent);
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
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/trapcock.wav"), 1, ATTN_NORM, 0);

		if (ent.client->ps.gunframe == 11)
		{
			if (!ent.client->trap_framenum)
			{
				ent.client->trap_framenum = (gtime) (level.framenum + (GRENADE_TIMER + 0.2f) * BASE_FRAMERATE);
				ent.client->weapon_sound = gi.soundindex("weapons/traploop.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent.client->trap_blew_up && level.framenum >= ent.client->trap_framenum)
			{
				ent.client->weapon_sound = SOUND_NONE;
				weapon_trap_fire(ent, true);
				ent.client->trap_blew_up = true;
			}

			if (ent.client->buttons & BUTTON_ATTACK)
				return;

			if (ent.client->trap_blew_up)
			{
				if (level.framenum >= ent.client->trap_framenum)
				{
					ent.client->ps.gunframe = 15;
					ent.client->trap_blew_up = false;
				}
				else
					return;
			}
		}

		if (ent.client->ps.gunframe == 12)
		{
			ent.client->weapon_sound = SOUND_NONE;
			weapon_trap_fire(ent, false);
			if (ent.client->pers.inventory[ent.client->ammo_index] == 0)
				NoAmmoWeaponChange(ent);
		}

		if ((ent.client->ps.gunframe == 15) && (level.framenum < ent.client->trap_framenum))
			return;

		ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == 16)
		{
			ent.client->trap_framenum = 0;
			ent.client->weaponstate = WEAPON_READY;
		}
	}
}
#endif
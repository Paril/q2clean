#include "config.h"
#include "game/entity_types.h"
#include "lib/gi.h"
#include "lib/math/random.h"
#include "game/util.h"
#include "game/weaponry/handgrenade.h"
#include "game/player_frames.h"
#include "weaponry.h"

#ifdef THE_RECKONING
#include "game/xatrix/weaponry/trap.h"
#endif

bool		is_quad;
muzzleflash	is_silenced;

int32_t	damage_multiplier;

void P_DamageModifier(entity &ent)
{
	is_quad = false;
	damage_multiplier = 1;

#ifdef CTF
	static gitem_t *tech_strength = 0;

	if (!tech_strength)
		tech_strength = FindItemByClassname("item_tech2");

	if (ent.client.pers.inventory[tech_strength.id])
	{
		damage_multiplier *= 2;
		is_quad = true;
	}
#endif

	if (ent.client->quad_framenum > level.framenum)
	{
		damage_multiplier *= 4;
		is_quad = true;

#ifdef GROUND_ZERO
		if (dmflags & DF_NO_STACK_DOUBLE)
			return;
#endif
	}

#ifdef GROUND_ZERO
	if (ent.client->double_framenum > level.framenum)
	{
		damage_multiplier *= 2;
		is_quad = true;
	}
#endif
}

#ifdef SINGLE_PLAYER
entity_type ET_PLAYER_NOISE("player_noise");

void PlayerNoise(entity &who, vector where, player_noise type)
{
	if (type == PNOISE_WEAPON)
	{
		if (who.client->silencer_shots)
		{
			who.client->silencer_shots--;
			return;
		}
	}

	if (deathmatch)
		return;

	if (who.flags & FL_NOTARGET)
		return;

#ifdef GROUND_ZERO
	if (who.flags & FL_DISGUISED)
	{
		if (type != PNOISE_WEAPON)
			return;

		level.disguise_violator = who;
		level.disguise_violation_framenum = level.framenum + 5;
	}
#endif

	if (!who.mynoise.has_value())
	{
		{
			entity &noise = G_Spawn();
			noise.type = ET_PLAYER_NOISE;
			noise.bounds = bbox::sized(8.f);
			noise.owner = who;
			noise.svflags = SVF_NOCLIENT;
			who.mynoise = noise;
		}

		{
			entity &noise = G_Spawn();
			noise.type = ET_PLAYER_NOISE;
			noise.bounds = bbox::sized(8.f);
			noise.owner = who;
			noise.svflags = SVF_NOCLIENT;
			who.mynoise2 = noise;
		}
	}

	entityref noise;

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who.mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else
	{ // type == PNOISE_IMPACT
		noise = who.mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	noise->origin = where;
	noise->absbounds = noise->bounds.offsetted(where);
	noise->last_sound_framenum = level.framenum;
	gi.linkentity(noise);
}
#endif

void ChangeWeapon(entity &ent)
{
	int i;

	if (ent.client->grenade_framenum)
	{
		ent.client->grenade_framenum = level.framenum;
		ent.client->weapon_sound = SOUND_NONE;
		ent.client->pers.weapon->weaponthink(ent);
		ent.client->grenade_framenum = 0;
	}

	ent.client->pers.lastweapon = ent.client->pers.weapon;
	ent.client->pers.weapon = ent.client->newweapon;
	ent.client->newweapon = 0;
	ent.client->machinegun_shots = 0;

	// set visible model
	if (ent.modelindex == MODEL_PLAYER)
	{
		if (ent.client->pers.weapon.has_value())
			i = ent.client->pers.weapon->vwep_id << 8;
		else
			i = 0;
		ent.skinnum = (ent.number - 1) | i;
	}

	if (ent.client->pers.weapon->ammo)
		ent.client->ammo_index = ent.client->pers.weapon->ammo;
	else
		ent.client->ammo_index = ITEM_NONE;

	if (!ent.client->pers.weapon.has_value())
	{
		// dead
		ent.client->ps.gunindex = MODEL_NONE;
		return;
	}

	ent.client->weaponstate = WEAPON_ACTIVATING;
	ent.client->ps.gunframe = 0;
	ent.client->ps.gunindex = gi.modelindex(ent.client->pers.weapon->view_model);

	ent.client->anim_priority = ANIM_PAIN;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.frame = FRAME_crpain1;
		ent.client->anim_end = FRAME_crpain4;
	}
	else
	{
		ent.frame = FRAME_pain301;
		ent.client->anim_end = FRAME_pain304;
	}
}

void NoAmmoWeaponChange(entity &ent)
{
	auto &inventory = ent.client->pers.inventory;

	if (inventory[ITEM_SLUGS] && inventory[ITEM_RAILGUN])
		ent.client->newweapon = ITEM_RAILGUN;
#ifdef THE_RECKONING
	else if (inventory[ITEM_MAGSLUG] && inventory[ITEM_PHALANX])
		ent.client->newweapon = ITEM_PHALANX;
	else if (inventory[ITEM_CELLS] && inventory[ITEM_BOOMER])
		ent.client->newweapon = ITEM_BOOMER;
#endif
#ifdef GROUND_ZERO
	else if (inventory[ITEM_CELLS] >= 2 && inventory[ITEM_PLASMA_BEAM])
		ent.client->newweapon = ITEM_PLASMA_BEAM;
	else if (inventory[ITEM_FLECHETTES] && inventory[ITEM_ETF_RIFLE])
		ent.client->newweapon = ITEM_ETF_RIFLE;
#endif
	else if (inventory[ITEM_CELLS] && inventory[ITEM_HYPERBLASTER])
		ent.client->newweapon = ITEM_HYPERBLASTER;
	else if (inventory[ITEM_BULLETS] && inventory[ITEM_CHAINGUN])
		ent.client->newweapon = ITEM_CHAINGUN;
	else if (inventory[ITEM_BULLETS] && inventory[ITEM_MACHINEGUN])
		ent.client->newweapon = ITEM_MACHINEGUN;
	else if (inventory[ITEM_SHELLS] > 1 && inventory[ITEM_SUPER_SHOTGUN])
		ent.client->newweapon = ITEM_SUPER_SHOTGUN;
	else if (inventory[ITEM_SHELLS] && inventory[ITEM_SHOTGUN])
		ent.client->newweapon = ITEM_SHOTGUN;
	else
		ent.client->newweapon = ITEM_BLASTER;
}

void Think_Weapon(entity &ent)
{
	// if just died, put the weapon away
	if (ent.health < 1)
	{
		ent.client->newweapon = ITEM_NONE;
		ChangeWeapon(ent);
	}

	// call active weapon think routine
	if (ent.client->pers.weapon->weaponthink)
	{
		P_DamageModifier(ent);
		is_silenced = (ent.client->silencer_shots) ? MZ_SILENCED : MZ_NONE;

		ent.client->pers.weapon->weaponthink(ent);
#ifdef THE_RECKONING
		if (ent.client->quadfire_framenum > level.framenum)
			ent.client->pers.weapon->weaponthink(ent);
#endif
#ifdef CTF
		if (CTFApplyHaste(ent))
			ent.client.pers.weapon->weaponthink(ent);
#endif
	}
}

void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, G_WeaponFrameFunc is_pause_frame, G_WeaponFrameFunc is_fire_frame, fire_func fire)
{
	const int32_t FRAME_FIRE_FIRST = FRAME_ACTIVATE_LAST + 1;
	const int32_t FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;
	const int32_t FRAME_DEACTIVATE_FIRST = FRAME_IDLE_LAST + 1;

	if (ent.deadflag || ent.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.client->weaponstate == WEAPON_DROPPING)
	{
		if (ent.client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon(ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent.client->ps.gunframe) == 4)
		{
			ent.client->anim_priority = ANIM_REVERSE;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.frame = FRAME_crpain4 + 1;
				ent.client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent.frame = FRAME_pain304 + 1;
				ent.client->anim_end = FRAME_pain301;
			}
		}

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent.client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent.client->weaponstate = WEAPON_READY;
			ent.client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->newweapon && (ent.client->weaponstate != WEAPON_FIRING))
	{
		ent.client->weaponstate = WEAPON_DROPPING;
		ent.client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent.client->anim_priority = ANIM_REVERSE;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.frame = FRAME_crpain4 + 1;
				ent.client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent.frame = FRAME_pain304 + 1;
				ent.client->anim_end = FRAME_pain301;

			}
		}
		return;
	}

	if (ent.client->weaponstate == WEAPON_READY)
	{
		if (((ent.client->latched_buttons | ent.client->buttons) & BUTTON_ATTACK))
		{
			ent.client->latched_buttons &= ~BUTTON_ATTACK;
			if (!ent.client->ammo_index ||
				(ent.client->pers.inventory[ent.client->ammo_index] >= ent.client->pers.weapon->quantity))
			{
				ent.client->ps.gunframe = FRAME_FIRE_FIRST;
				ent.client->weaponstate = WEAPON_FIRING;

				// start the animation
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
			else
			{
				if (level.framenum >= ent.pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"));
					ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
				}
				NoAmmoWeaponChange(ent);
			}
		}
		else
		{
			if (ent.client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent.client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (is_pause_frame(ent.client->ps.gunframe) && (Q_rand() & 15))
				return;

			ent.client->ps.gunframe++;
			return;
		}
	}

	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		bool fired = false;

		if (is_fire_frame(ent.client->ps.gunframe))
		{
#ifdef CTF
			if (CTFApplyStrengthSound(ent)) {}
			else
#endif
			if (ent.client->quad_framenum > level.framenum)
				gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"));
#ifdef GROUND_ZERO
			else if (ent.client->double_framenum > level.framenum)
				gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"));
#endif

#ifdef CTF
			CTFApplyHasteSound(ent);
#endif

			fire(ent);

			fired = true;
		}

		if (!fired)
			ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent.client->weaponstate = WEAPON_READY;
	}
}

void Throw_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_THROW_SOUND, stringlit throw_sound, int32_t FRAME_THROW_HOLD, stringlit weapon_sound, int32_t FRAME_THROW_FIRE, G_WeaponFrameFunc is_pause_frame, void (*fire)(entity &, bool), bool explode_in_hand)
{
	const int32_t FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;

	if ((ent.client->newweapon) && (ent.client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent.client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent.client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent.client->weaponstate = WEAPON_READY;
			ent.client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent.client->ps.gunframe++;
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
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"));
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

		if (is_pause_frame(ent.client->ps.gunframe) && (Q_rand() & 15))
			return;

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		if (ent.client->ps.gunframe == FRAME_THROW_SOUND)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex(throw_sound));

		if (ent.client->ps.gunframe == FRAME_THROW_HOLD)
		{
			if (!ent.client->grenade_framenum)
			{
				ent.client->grenade_framenum = level.framenum + (gtime)((GRENADE_TIMER + 0.2) * BASE_FRAMERATE);

				if (weapon_sound)
					ent.client->weapon_sound = gi.soundindex(weapon_sound);
			}

			// they waited too long, detonate it in their hand
			if (explode_in_hand && !ent.client->grenade_blew_up && level.framenum >= ent.client->grenade_framenum)
			{
				ent.client->weapon_sound = SOUND_NONE;
				fire (ent, true);
				ent.client->grenade_blew_up = true;
			}
			else if (ent.client->buttons & BUTTON_ATTACK)
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
			
			if (!ent.client->pers.inventory[ent.client->ammo_index])
				NoAmmoWeaponChange (ent);
		}
	}
}
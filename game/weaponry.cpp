#include "config.h"
#include "game/entity_types.h"
#include "lib/gi.h"
#include "lib/math/random.h"
#include "game/util.h"
#include "game/weaponry/handgrenade.h"
#include "game/player_frames.h"
#include "weaponry.h"

bool			is_quad;
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
		if (dmflags.intVal & DF_NO_STACK_DOUBLE)
			return;
#endif
	}

#ifdef GROUND_ZERO
	if (ent.client.double_framenum > level.framenum)
	{
		damage_multiplier *= 2;
		is_quad = true;
	}
#endif
}

#ifdef SINGLE_PLAYER
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

	if ((int32_t) deathmatch)
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
			noise.mins = { -8, -8, -8 };
			noise.maxs = { 8, 8, 8 };
			noise.owner = who;
			noise.svflags = SVF_NOCLIENT;
			who.mynoise = noise;
		}

		{
			entity &noise = G_Spawn();
			noise.type = ET_PLAYER_NOISE;
			noise.mins = { -8, -8, -8 };
			noise.maxs = { 8, 8, 8 };
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

	noise->s.origin = where;
	noise->absmin = where + noise->maxs;
	noise->absmax = where + noise->maxs;
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
		weapon_grenade_fire(ent, false);
		ent.client->grenade_framenum = 0;
	}

#ifdef THE_RECKONING
	if (ent.client.trap_framenum)
	{
		ent.client.trap_framenum = level.framenum;
		ent.client.weapon_sound = 0;
		weapon_trap_fire(ent, false);
		ent.client.trap_framenum = 0;
	}
#endif

	ent.client->pers.lastweapon = ent.client->pers.weapon;
	ent.client->pers.weapon = ent.client->newweapon;
	ent.client->newweapon = 0;
	ent.client->machinegun_shots = 0;

	// set visible model
	if (ent.s.modelindex == MODEL_PLAYER)
	{
		if (ent.client->pers.weapon.has_value())
			i = ent.client->pers.weapon->vwep_id << 8;
		else
			i = 0;
		ent.s.skinnum = (ent.s.number - 1) | i;
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
		ent.s.frame = FRAME_crpain1;
		ent.client->anim_end = FRAME_crpain4;
	}
	else
	{
		ent.s.frame = FRAME_pain301;
		ent.client->anim_end = FRAME_pain304;
	}
}

void NoAmmoWeaponChange(entity &ent)
{
	auto &inventory = ent.client->pers.inventory;

	if (inventory[ITEM_SLUGS] && inventory[ITEM_RAILGUN])
		ent.client->newweapon = ITEM_RAILGUN;
#ifdef THE_RECKONING
	else if (ent.client.pers.inventory[FindItem("mag slug")->id]
		&& ent.client.pers.inventory[FindItem("phalanx")->id])
		ent.client.newweapon = FindItem("phalanx");
	else if (ent.client.pers.inventory[FindItem("cells")->id]
		&& ent.client.pers.inventory[FindItem("ionripper")->id])
		ent.client.newweapon = FindItem("ionripper");
#endif
#ifdef GROUND_ZERO
	else if ((ent.client.pers.inventory[FindItem("cells")->id] >= 2)
		&& ent.client.pers.inventory[FindItem("Plasma Beam")->id])
		ent.client.newweapon = FindItem("Plasma Beam");
	else if (ent.client.pers.inventory[FindItem("flechettes")->id]
		&& ent.client.pers.inventory[FindItem("etf rifle")->id])
		ent.client.newweapon = FindItem("etf rifle");
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
		if (ent.client.quadfire_framenum > level.framenum)
			ent.client.pers.weapon->weaponthink(ent);
#endif
#ifdef CTF
		if (CTFApplyHaste(ent))
			ent.client.pers.weapon->weaponthink(ent);
#endif
	}
}

void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, std::initializer_list<int32_t> is_pause_frame, std::initializer_list<int32_t> is_fire_frame, fire_func *fire)
{
	const int32_t FRAME_FIRE_FIRST = FRAME_ACTIVATE_LAST + 1;
	const int32_t FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;
	const int32_t FRAME_DEACTIVATE_FIRST = FRAME_IDLE_LAST + 1;

	if (ent.deadflag || ent.s.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
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
				ent.s.frame = FRAME_crpain4 + 1;
				ent.client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent.s.frame = FRAME_pain304 + 1;
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
				ent.s.frame = FRAME_crpain4 + 1;
				ent.client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent.s.frame = FRAME_pain304 + 1;
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
					ent.s.frame = FRAME_crattak1 - 1;
					ent.client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent.s.frame = FRAME_attack1 - 1;
					ent.client->anim_end = FRAME_attack8;
				}
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
		}
		else
		{
			if (ent.client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent.client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			for (auto &frame : is_pause_frame)
				if (frame == ent.client->ps.gunframe && (Q_rand() & 15))
					return;

			ent.client->ps.gunframe++;
			return;
		}
	}

	if (ent.client->weaponstate == WEAPON_FIRING)
	{
		bool fired = false;

		for (auto &frame : is_fire_frame)
			if (frame == ent.client->ps.gunframe)
			{
#ifdef CTF
				if (CTFApplyStrengthSound(ent)) {}
				else
#endif
					if (ent.client->quad_framenum > level.framenum)
						gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
#ifdef GROUND_ZERO
					else if (ent.client.double_framenum > level.framenum)
						gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
#endif

#ifdef CTF
				CTFApplyHasteSound(ent);
#endif

				fire(ent);

				fired = true;
				break;
			}

		if (!fired)
			ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent.client->weaponstate = WEAPON_READY;
	}
}
#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "gweapon.h"
#include "game.h"
#include "itemlist.h"
#include "pweapon.h"
#include "view.h"
#include "m_player.h"

bool		is_quad;
muzzleflash	is_silenced;

#ifdef THE_RECKONING
PROGS_LOCAL void(entity, bool) weapon_trap_fire;
#endif

int32_t	damage_multiplier;

// Paril: from GZ, but handy to keep here
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

	if (ent.client->g.quad_framenum > level.framenum)
	{
		damage_multiplier *= 4;
		is_quad = true;

#ifdef GROUND_ZERO
		// if we're quad and DF_NO_STACK_DOUBLE is on, return now.
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

static void weapon_grenade_fire(entity &, bool);

#ifdef SINGLE_PLAYER
/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void(entity who, vector where, player_noise_t type) PlayerNoise =
{
	entity	noise;

	if (type == PNOISE_WEAPON)
	{
		if (who.client.silencer_shots)
		{
			who.client.silencer_shots--;
			return;
		}
	}

	if (deathmatch.intVal)
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

	if (!who.mynoise)
	{
		noise = G_Spawn();
		noise.classname = "player_noise";
		noise.mins = '-8 -8 -8';
		noise.maxs = '8 8 8';
		noise.owner = who;
		noise.svflags = SVF_NOCLIENT;
		who.mynoise = noise;

		noise = G_Spawn();
		noise.classname = "player_noise";
		noise.mins = '-8 -8 -8';
		noise.maxs = '8 8 8';
		noise.owner = who;
		noise.svflags = SVF_NOCLIENT;
		who.mynoise2 = noise;
	}

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

	noise.s.origin = where;
	noise.absmin = where + noise.maxs;
	noise.absmax = where + noise.maxs;
	noise.last_sound_framenum = level.framenum;
	gi.linkentity(noise);
}
#endif

bool Pickup_Weapon(entity &ent, entity &other)
{
	const gitem_id index = ent.g.item->id;

#ifdef SINGLE_PLAYER
	if (((dmflags.intVal & DF_WEAPONS_STAY) || coop.intVal) &&
#else
	if (((dm_flags)dmflags & DF_WEAPONS_STAY) &&
#endif
		other.client->g.pers.inventory[index])
	{
		if (!(ent.g.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;   // leave the weapon for others to pickup
	}

	other.client->g.pers.inventory[index]++;

	if (!(ent.g.spawnflags & DROPPED_ITEM))
	{
		// give them some ammo with it
		if (ent.g.item->ammo)
		{
			const gitem_t &ammo = GetItemByIndex(ent.g.item->ammo);
			Add_Ammo(other, ammo, ammo.quantity);
		}

		if (!(ent.g.spawnflags & DROPPED_PLAYER_ITEM))
		{
#ifdef SINGLE_PLAYER
			if (deathmatch.intVal)
			{
#endif
				if ((dm_flags)dmflags & DF_WEAPONS_STAY)
					ent.g.flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30);
#ifdef SINGLE_PLAYER
			}
			if (coop.intVal)
				ent.flags |= FL_RESPAWN;
#endif
		}
	}

	if (other.client->g.pers.weapon != ent.g.item && (other.client->g.pers.inventory[index] == 1) && (
#ifdef SINGLE_PLAYER
		!deathmatch.intVal || 
#endif
		other.client->g.pers.weapon->id == ITEM_BLASTER))
		other.client->g.newweapon = ent.g.item;

	return true;
}

void ChangeWeapon(entity &ent)
{
	int i;

	if (ent.client->g.grenade_framenum)
	{
		ent.client->g.grenade_framenum = level.framenum;
		ent.client->g.weapon_sound = SOUND_NONE;
		weapon_grenade_fire(ent, false);
		ent.client->g.grenade_framenum = 0;
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

	ent.client->g.pers.lastweapon = ent.client->g.pers.weapon;
	ent.client->g.pers.weapon = ent.client->g.newweapon;
	ent.client->g.newweapon = 0;
	ent.client->g.machinegun_shots = 0;

	// set visible model
	if (ent.s.modelindex == MODEL_PLAYER)
	{
		if (ent.client->g.pers.weapon.has_value())
			i = ent.client->g.pers.weapon->vwep_id << 8;
		else
			i = 0;
		ent.s.skinnum = (ent.s.number - 1) | i;
	}

	if (ent.client->g.pers.weapon->ammo)
		ent.client->g.ammo_index = ent.client->g.pers.weapon->ammo;
	else
		ent.client->g.ammo_index = ITEM_NONE;

	if (!ent.client->g.pers.weapon.has_value())
	{
		// dead
		ent.client->ps.gunindex = MODEL_NONE;
		return;
	}

	ent.client->g.weaponstate = WEAPON_ACTIVATING;
	ent.client->ps.gunframe = 0;
	ent.client->ps.gunindex = gi.modelindex(ent.client->g.pers.weapon->view_model);

	ent.client->g.anim_priority = ANIM_PAIN;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crpain1;
		ent.client->g.anim_end = FRAME_crpain4;
	}
	else
	{
		ent.s.frame = FRAME_pain301;
		ent.client->g.anim_end = FRAME_pain304;
	}
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(entity &ent)
{
	auto &inventory = ent.client->g.pers.inventory;

	if (inventory[ITEM_SLUGS] && inventory[ITEM_RAILGUN])
		ent.client->g.newweapon = ITEM_RAILGUN;
#ifdef THE_RECKONING
	else if (ent.client.pers.inventory[FindItem("mag slug")->id]
		&& ent.client.pers.inventory[FindItem("phalanx")->id])
		ent.client.newweapon = FindItem("phalanx");
	else if ( ent.client.pers.inventory[FindItem("cells")->id]
		&& ent.client.pers.inventory[FindItem("ionripper")->id])
		ent.client.newweapon = FindItem("ionripper");
#endif
#ifdef GROUND_ZERO
	else if ((ent.client.pers.inventory[FindItem("cells")->id] >= 2)
		&& ent.client.pers.inventory[FindItem("Plasma Beam")->id])
		ent.client.newweapon = FindItem ("Plasma Beam");
	else if (ent.client.pers.inventory[FindItem("flechettes")->id]
		&&  ent.client.pers.inventory[FindItem("etf rifle")->id])
		ent.client.newweapon = FindItem ("etf rifle");
#endif
	else if (inventory[ITEM_CELLS] && inventory[ITEM_HYPERBLASTER])
		ent.client->g.newweapon = ITEM_HYPERBLASTER;
	else if (inventory[ITEM_BULLETS] && inventory[ITEM_CHAINGUN])
		ent.client->g.newweapon = ITEM_CHAINGUN;
	else if (inventory[ITEM_BULLETS] && inventory[ITEM_MACHINEGUN])
		ent.client->g.newweapon = ITEM_MACHINEGUN;
	else if (inventory[ITEM_SHELLS] > 1 && inventory[ITEM_SUPER_SHOTGUN])
		ent.client->g.newweapon = ITEM_SUPER_SHOTGUN;
	else if (inventory[ITEM_SHELLS] && inventory[ITEM_SHOTGUN])
		ent.client->g.newweapon = ITEM_SHOTGUN;
	else
		ent.client->g.newweapon = ITEM_BLASTER;
}

void Think_Weapon(entity &ent)
{
	// if just died, put the weapon away
	if (ent.g.health < 1)
	{
		ent.client->g.newweapon = ITEM_NONE;
		ChangeWeapon(ent);
	}

	// call active weapon think routine
	if (ent.client->g.pers.weapon->weaponthink)
	{
		P_DamageModifier(ent);
		is_silenced = (ent.client->g.silencer_shots) ? MZ_SILENCED : MZ_NONE;

		ent.client->g.pers.weapon->weaponthink(ent);
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

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
enum use_weap_result : uint8_t 
{
	USEWEAP_OK,
	USEWEAP_NO_AMMO,
	USEWEAP_NOT_ENOUGH_AMMO,
	USEWEAP_IS_ACTIVE,
	USEWEAP_OUT_OF_ITEM
};

static use_weap_result CanUseWeapon(entity &ent, const gitem_t &it)
{
	if (it == ent.client->g.pers.weapon)
		return USEWEAP_IS_ACTIVE;

	if (!ent.client->g.pers.inventory[it.id])
		return USEWEAP_OUT_OF_ITEM;
	
	if (it.ammo && !(int32_t)g_select_empty && !(it.flags & IT_AMMO))
	{
		if (!ent.client->g.pers.inventory[it.ammo])
			return USEWEAP_NO_AMMO;
		else if (ent.client->g.pers.inventory[it.ammo] < it.quantity)
			return USEWEAP_NOT_ENOUGH_AMMO;
	}

	return USEWEAP_OK;
};

#ifdef THE_RECKONING
inline std::initializer_list<gitem_id> rail_chain = { ITEM_RAILGUN, ITEM_PHALANX };
#endif

#ifdef GROUND_ZERO
inline std::initializer_list<gitem_id> grenadelauncher_chain = { ITEM_GRENADE_LAUNCHER, ITEM_PROX_LAUNCHER };
inline std::initializer_list<gitem_id> machinegun_chain = { ITEM_MACHINEGUN, ITEM_ETF_RIFLE };
inline std::initializer_list<gitem_id> bfg_chain = { ITEM_BFG, ITEM_DISRUPTOR };
#endif

#if defined(GROUND_ZERO) && defined(GRAPPLE)
inline std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_CHAINFIST, ITEM_GRAPPLE };
#elif defined(GRAPPLE)
inline std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_GRAPPLE };
#elif defined(GROUND_ZERO)
inline std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_CHAINFIST };
#endif

#if defined(GROUND_ZERO) && defined(THE_RECKONING)
inline std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TRAP, ITEM_TESLA};
inline std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_BOOMER, ITEM_PLASMA_BEAM };
#elif defined(GROUND_ZERO)
inline std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TESLA };
inline std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_PLASMA_BEAM };
#elif defined(THE_RECKONING)
inline std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TRAP };
inline std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_BOOMER };
#endif

static inline bool GetChain(entity &ent [[maybe_unused]], const gitem_t &it [[maybe_unused]], std::initializer_list<gitem_id> &chain [[maybe_unused]])
{
#if defined(GROUND_ZERO) || defined(GRAPPLE)
	if (it.id == ITEM_BLASTER)
		chain = blaster_chain;
#endif
#if defined(GROUND_ZERO) || defined(THE_RECKONING)
	else if (it.id == ITEM_HYPERBLASTER)
		chain = hyperblaster_chain;
	else if (it.id == ITEM_GRENADES)
		chain = grenade_chain;
#endif
#if defined(GROUND_ZERO)
	else if (it.id == ITEM_GRENADE_LAUNCHER)
		chain = grenadelauncher_chain;
	else if (it.id == ITEM_MACHINEGUN)
		chain = machinegun_chain;
	else if (it.id == ITEM_BFG)
		chain = bfg_chain;
#endif
#if defined(THE_RECKONING)
	else if (it.id == ITEM_RAILGUN)
		chain = rail_chain;
#endif
#if defined(GROUND_ZERO) || defined(THE_RECKONING) || defined(GRAPPLE)
	else
#endif
		return false;
#if defined(GROUND_ZERO) || defined(THE_RECKONING) || defined(GRAPPLE)

	return true;
#endif
}

void Use_Weapon(entity &ent, const gitem_t &it)
{
	std::initializer_list<gitem_id> chain;

	// we have a weapon chain; find our current position in this chain
	if (GetChain(ent, it, chain))
	{
		size_t chain_start = 0;

		for (const gitem_id *c = chain.begin(); c != chain.end(); c++)
		{
			const gitem_t &cit = GetItemByIndex(*c);
	
			if (ent.client->g.pers.weapon == cit || ent.client->g.newweapon == cit)
			{
				chain_start = (c - chain.begin()) + 1;
				break;
			}
		}

		// find the next weapon we can equip that doesn't return false
		for (size_t cid = 0; cid < chain.size(); cid++)
		{
			const gitem_t &cit = GetItemByIndex(*(chain.begin() + ((chain_start + cid) % chain.size())));
			
			// got it!
			if (CanUseWeapon(ent, cit) == USEWEAP_OK)
			{
				ent.client->g.newweapon = cit;
				return;
			}
		}

		// fall back to default warning message
	}
	
	const use_weap_result result = CanUseWeapon(ent, it);
	const gitem_t &ammo_item = GetItemByIndex(it.ammo);

	switch (result)
	{
	case USEWEAP_IS_ACTIVE:
		return;
	case USEWEAP_NO_AMMO:
		gi.cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	case USEWEAP_NOT_ENOUGH_AMMO:
		gi.cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	}

	// change to this weapon when down
	ent.client->g.newweapon = it;
}

void Drop_Weapon(entity &ent, const gitem_t &it)
{
	if ((dm_flags)dmflags & DF_WEAPONS_STAY)
		return;

	// see if we're already using it
	if (((it == ent.client->g.pers.weapon) || (it == ent.client->g.newweapon)) && (ent.client->g.pers.inventory[it.id] == 1))
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, it);
	
	ent.client->g.pers.inventory[it.id]--;
}

/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
void Weapon_Generic(entity &ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST, std::initializer_list<int32_t> is_pause_frame, std::initializer_list<int32_t> is_fire_frame, fire_func *fire)
{
	const int32_t FRAME_FIRE_FIRST	= FRAME_ACTIVATE_LAST + 1;
	const int32_t FRAME_IDLE_FIRST	= FRAME_FIRE_LAST + 1;
	const int32_t FRAME_DEACTIVATE_FIRST = FRAME_IDLE_LAST + 1;

	if (ent.g.deadflag || ent.s.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.client->g.weaponstate == WEAPON_DROPPING)
	{
		if (ent.client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon(ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent.client->ps.gunframe) == 4)
		{
			ent.client->g.anim_priority = ANIM_REVERSE;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.s.frame = FRAME_crpain4 + 1;
				ent.client->g.anim_end = FRAME_crpain1;
			}
			else
			{
				ent.s.frame = FRAME_pain304 + 1;
				ent.client->g.anim_end = FRAME_pain301;
			}
		}

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->g.weaponstate == WEAPON_ACTIVATING)
	{
		if (ent.client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent.client->g.weaponstate = WEAPON_READY;
			ent.client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->g.newweapon && (ent.client->g.weaponstate != WEAPON_FIRING))
	{
		ent.client->g.weaponstate = WEAPON_DROPPING;
		ent.client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent.client->g.anim_priority = ANIM_REVERSE;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.s.frame = FRAME_crpain4 + 1;
				ent.client->g.anim_end = FRAME_crpain1;
			}
			else
			{
				ent.s.frame = FRAME_pain304 + 1;
				ent.client->g.anim_end = FRAME_pain301;

			}
		}
		return;
	}

	if (ent.client->g.weaponstate == WEAPON_READY)
	{
		if (((ent.client->g.latched_buttons | ent.client->g.buttons) & BUTTON_ATTACK))
		{
			ent.client->g.latched_buttons &= ~BUTTON_ATTACK;
			if (!ent.client->g.ammo_index || 
				(ent.client->g.pers.inventory[ent.client->g.ammo_index] >= ent.client->g.pers.weapon->quantity))
			{
				ent.client->ps.gunframe = FRAME_FIRE_FIRST;
				ent.client->g.weaponstate = WEAPON_FIRING;

				// start the animation
				ent.client->g.anim_priority = ANIM_ATTACK;
				if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent.s.frame = FRAME_crattak1 - 1;
					ent.client->g.anim_end = FRAME_crattak9;
				}
				else
				{
					ent.s.frame = FRAME_attack1 - 1;
					ent.client->g.anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.framenum >= ent.g.pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent.g.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
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

	if (ent.client->g.weaponstate == WEAPON_FIRING)
	{
		bool fired = false;

		for (auto &frame : is_fire_frame)
			if (frame == ent.client->ps.gunframe)
			{
#ifdef CTF
				if (CTFApplyStrengthSound(ent)) { }
				else
#endif
				if (ent.client->g.quad_framenum > level.framenum)
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
			ent.client->g.weaponstate = WEAPON_READY;
	}
}

/*
======================================================================

GRENADE

======================================================================
*/
static void weapon_grenade_fire(entity &ent, bool held)
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

	offset = { 8, 8, ent.g.viewheight - 8.f };
	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	timer = (ent.client->g.grenade_framenum - level.framenum) * FRAMETIME;
	speed = (int32_t)(GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	fire_grenade2(ent, start, forward, damage, speed, timer, radius, held);

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;

	ent.client->g.grenade_framenum = (gtime)(level.framenum + 1.0f * BASE_FRAMERATE);

	if (ent.g.deadflag || ent.s.modelindex != MODEL_PLAYER) // VWep animations screw up corpses
		return;

	if (ent.g.health <= 0)
		return;

	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.client->g.anim_priority = ANIM_ATTACK;
		ent.s.frame = FRAME_crattak1 - 1;
		ent.client->g.anim_end = FRAME_crattak3;
	}
	else
	{
		ent.client->g.anim_priority = ANIM_REVERSE;
		ent.s.frame = FRAME_wave08;
		ent.client->g.anim_end = FRAME_wave01;
	}
}

void Weapon_Grenade(entity &ent)
{
	if ((ent.client->g.newweapon) && (ent.client->g.weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent);
		return;
	}

	if (ent.client->g.weaponstate == WEAPON_ACTIVATING)
	{
		ent.client->g.weaponstate = WEAPON_READY;
		ent.client->ps.gunframe = 16;
		return;
	}

	if (ent.client->g.weaponstate == WEAPON_READY)
	{
		if (((ent.client->g.latched_buttons | ent.client->g.buttons) & BUTTON_ATTACK))
		{
			ent.client->g.latched_buttons &= ~BUTTON_ATTACK;
			if (ent.client->g.pers.inventory[ent.client->g.ammo_index])
			{
				ent.client->ps.gunframe = 1;
				ent.client->g.weaponstate = WEAPON_FIRING;
				ent.client->g.grenade_framenum = 0;
			}
			else
			{
				if (level.framenum >= ent.g.pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent.g.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
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

	if (ent.client->g.weaponstate == WEAPON_FIRING)
	{
		if (ent.client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent.client->ps.gunframe == 11)
		{
			if (!ent.client->g.grenade_framenum) {
				ent.client->g.grenade_framenum = (gtime)(level.framenum + (GRENADE_TIMER + 0.2f) * BASE_FRAMERATE);
				ent.client->g.weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent.client->g.grenade_blew_up && level.framenum >= ent.client->g.grenade_framenum)
			{
				ent.client->g.weapon_sound = SOUND_NONE;
				weapon_grenade_fire(ent, true);
				ent.client->g.grenade_blew_up = true;
			}

			if (ent.client->g.buttons & BUTTON_ATTACK)
				return;

			if (ent.client->g.grenade_blew_up)
			{
				if (level.framenum >= ent.client->g.grenade_framenum)
				{
					ent.client->ps.gunframe = 15;
					ent.client->g.grenade_blew_up = false;
				}
				else
					return;
			}
		}

		if (ent.client->ps.gunframe == 12)
		{
			ent.client->g.weapon_sound = SOUND_NONE;
			weapon_grenade_fire(ent, false);
		}

		if ((ent.client->ps.gunframe == 15) && (level.framenum < ent.client->g.grenade_framenum))
			return;

		ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == 16)
		{
			ent.client->g.grenade_framenum = 0;
			ent.client->g.weaponstate = WEAPON_READY;
		}
	}
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

static void weapon_grenadelauncher_fire(entity &ent)
{
	vector	offset;
	vector	forward, right;
	vector	start;
#ifdef GROUND_ZERO
	bool	is_prox = ent.client.pers.weapon->id == ITEM_PROX_LAUNCHER;
	int32_t	damage = is_prox ? 90 : 120;
#else
	int32_t	damage = 120;
#endif
	float	radius;

	radius = damage + 40.f;
	if (is_quad)
		damage *= damage_multiplier;

	offset = { 8, 8, ent.g.viewheight - 8.f };
	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	ent.client->g.kick_origin = forward * -2;
	ent.client->g.kick_angles[0] = -1.f;

#ifdef GROUND_ZERO
	if (is_prox)
		fire_prox (ent, start, forward, damage_multiplier, 600);
	else
#endif
		fire_grenade(ent, start, forward, damage, 600, 2.5f, radius);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;
}

void Weapon_GrenadeLauncher(entity &ent)
{
	Weapon_Generic(ent, 5, 16, 59, 64, { 34, 51, 59 }, { 6 }, weapon_grenadelauncher_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

static void Weapon_RocketLauncher_Fire(entity &ent)
{
	vector	offset, start;
	vector	forward, right;
	int32_t	damage;
	float	damage_radius;
	int32_t	radius_damage;

	damage = (int32_t)random(100.f, 120.f);
	radius_damage = 120;
	damage_radius = 120.f;
	if (is_quad) {
		damage *= damage_multiplier;
		radius_damage *= damage_multiplier;
	}

	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	ent.client->g.kick_origin = forward * -2;
	ent.client->g.kick_angles[0] = -1.f;

	offset = { 8, 8, ent.g.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);
	fire_rocket(ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;
}

void Weapon_RocketLauncher(entity &ent)
{
	Weapon_Generic(ent, 4, 12, 50, 54, { 25, 33, 42, 50 }, { 5 }, Weapon_RocketLauncher_Fire);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

static inline void Blaster_Fire(entity &ent, vector g_offset, int32_t damage, bool hyper, entity_effects effect)
{
	vector	forward, right;
	vector	start;
	vector	offset;

	if (is_quad)
		damage *= damage_multiplier;
	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);
	offset = { 24, 8, ent.g.viewheight - 8.f };
	offset += g_offset;
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	ent.client->g.kick_origin = forward * -2;
	ent.client->g.kick_angles[0] = -1.f;

	fire_blaster(ent, start, forward, damage, 1000, effect, hyper ? MOD_HYPERBLASTER : MOD_BLASTER, hyper);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
}

static void Weapon_Blaster_Fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	int32_t damage;
	if !(deathmatch.intVal)
		damage = 10;
	else
		damage = 15;

#else
	constexpr int32_t damage = 15;
#endif
	Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
	ent.client->ps.gunframe++;
}

void Weapon_Blaster(entity &ent)
{
	Weapon_Generic(ent, 4, 8, 52, 55, { 19, 32 }, { 5 }, Weapon_Blaster_Fire);
}

static void Weapon_HyperBlaster_Fire(entity &ent)
{
	ent.client->g.weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent.client->g.buttons & BUTTON_ATTACK))
		ent.client->ps.gunframe++;
	else
	{
		if (!ent.client->g.pers.inventory[ent.client->g.ammo_index])
		{
			if (level.framenum >= ent.g.pain_debounce_framenum)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent.g.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
			}
			NoAmmoWeaponChange(ent);
		}
		else
		{
			float rotation = (ent.client->ps.gunframe - 5) * (PI / 3);
			vector offset = { -4.f * sin(rotation), 0, 4.f * cos(rotation) };
			entity_effects effect;

			if ((ent.client->ps.gunframe == 6) || (ent.client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = EF_NONE;

#ifdef SINGLE_PLAYER
			int32_t damage;

			if !(deathmatch.intVal)
				damage = 20;
			else
				damage = 15;
				
#else
			constexpr int32_t damage = 15;
#endif
			Blaster_Fire(ent, offset, damage, true, effect);

			if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
				ent.client->g.pers.inventory[ent.client->g.ammo_index]--;

			ent.client->g.anim_priority = ANIM_ATTACK;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.s.frame = FRAME_crattak1 - 1;
				ent.client->g.anim_end = FRAME_crattak9;
			}
			else
			{
				ent.s.frame = FRAME_attack1 - 1;
				ent.client->g.anim_end = FRAME_attack8;
			}
		}

		ent.client->ps.gunframe++;
		
		if (ent.client->ps.gunframe == 12 && ent.client->g.pers.inventory[ent.client->g.ammo_index])
			ent.client->ps.gunframe = 6;
	}

	if (ent.client->ps.gunframe == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent.client->g.weapon_sound = SOUND_NONE;
	}
}

void Weapon_HyperBlaster(entity &ent)
{
	Weapon_Generic(ent, 5, 20, 49, 53, {}, { 6, 7, 8, 9, 10, 11 }, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

static void Machinegun_Fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	angles;
	int32_t	damage = 8;
	int32_t	kick = 2;
	vector	offset;

	if (!(ent.client->g.buttons & BUTTON_ATTACK)) {
		ent.client->g.machinegun_shots = 0;
		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->ps.gunframe == 5)
		ent.client->ps.gunframe = 4;
	else
		ent.client->ps.gunframe = 5;

	if (ent.client->g.pers.inventory[ent.client->g.ammo_index] < 1)
	{
		ent.client->ps.gunframe = 6;
		if (level.framenum >= ent.g.pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent.g.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
		}
		NoAmmoWeaponChange(ent);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent.client->g.kick_origin[1] = random(-0.35f, 0.35f);
	ent.client->g.kick_angles[1] = random(-0.7f, 0.7f);
	ent.client->g.kick_origin[2] = random(-0.35f, 0.35f);
	ent.client->g.kick_angles[2] = random(-0.7f, 0.7f);
	
	ent.client->g.kick_origin[0] = random(-0.35f, 0.35f);
	ent.client->g.kick_angles[0] = ent.client->g.machinegun_shots * -1.5f;
#ifdef SINGLE_PLAYER

	// raise the gun as it is firing
	if (!deathmatch.intVal)
	{
		ent.client.machinegun_shots++;
		if (ent.client.machinegun_shots > 9)
			ent.client.machinegun_shots = 9;
	}
#endif

	// get start / end positions
	angles = ent.client->g.v_angle + ent.client->g.kick_angles;
	AngleVectors(angles, &forward, &right, nullptr);
	offset = { 0, 8, ent.g.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);
	
	fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;

	ent.client->g.anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crattak1 - (int)random(0.25f, 1.25f);
		ent.client->g.anim_end = FRAME_crattak9;
	}
	else
	{
		ent.s.frame = FRAME_attack1 - (int)random(0.25f, 1.25f);
		ent.client->g.anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun(entity &ent)
{
	Weapon_Generic(ent, 3, 5, 45, 49, { 23, 45 }, { 4, 5 }, Machinegun_Fire);
}

static void Chaingun_Fire(entity &ent)
{
	vector	start = vec3_origin;
	vector	forward, right, up;
	float	r, u;
	vector	offset;
	int32_t	kick = 2;
#ifdef SINGLE_PLAYER
	int32_t	damage;

	if !(deathmatch.intVal)
		damage = 8;
	else
		damage = 6;
#else
	int32_t	damage = 6;
#endif

	if (ent.client->ps.gunframe == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent.client->ps.gunframe == 14) && !(ent.client->g.buttons & BUTTON_ATTACK))
	{
		ent.client->ps.gunframe = 32;
		ent.client->g.weapon_sound = SOUND_NONE;
		return;
	}
	else if ((ent.client->ps.gunframe == 21) && (ent.client->g.buttons & BUTTON_ATTACK)
			   && ent.client->g.pers.inventory[ent.client->g.ammo_index])
		ent.client->ps.gunframe = 15;
	else
		ent.client->ps.gunframe++;

	if (ent.client->ps.gunframe == 22)
	{
		ent.client->g.weapon_sound = SOUND_NONE;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
		ent.client->g.weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	ent.client->g.anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crattak1 - (ent.client->ps.gunframe & 1);
		ent.client->g.anim_end = FRAME_crattak9;
	}
	else
	{
		ent.s.frame = FRAME_attack1 - (ent.client->ps.gunframe & 1);
		ent.client->g.anim_end = FRAME_attack8;
	}

	int32_t shots;

	if (ent.client->ps.gunframe <= 9)
		shots = 1;
	else if (ent.client->ps.gunframe <= 14)
	{
		if (ent.client->g.buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent.client->g.pers.inventory[ent.client->g.ammo_index] < shots)
		shots = ent.client->g.pers.inventory[ent.client->g.ammo_index];

	if (!shots)
	{
		if (level.framenum >= ent.g.pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent.g.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
		}
		NoAmmoWeaponChange(ent);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}
	
	ent.client->g.kick_origin = randomv({ -0.35f, -0.35f, -0.35f }, { 0.35f, 0.35f, 0.35f });
	ent.client->g.kick_angles = randomv({ -0.7f, -0.7f, -0.7f }, { 0.7f, 0.7f, 0.7f });

	for (int32_t i = 0; i < shots; i++)
	{
		// get start / end positions
		AngleVectors(ent.client->g.v_angle, &forward, &right, &up);
		r = random(3.f, 11.f);
		u = random(-4.f, 4.f);
		offset = { 0, r, u + ent.g.viewheight - 8 };
		start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

		fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte((uint8_t)((MZ_CHAINGUN1 + shots - 1) | is_silenced));
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index] -= shots;
}

void Weapon_Chaingun(entity &ent)
{
	Weapon_Generic(ent, 4, 31, 61, 64, { 38, 43, 51, 61 }, { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 }, Chaingun_Fire);
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

static void weapon_shotgun_fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	offset;
	int32_t	damage = 4;
	int32_t	kick = 8;

	if (ent.client->ps.gunframe == 9)
	{
		ent.client->ps.gunframe++;
		return;
	}

	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	ent.client->g.kick_origin = forward * -2;
	ent.client->g.kick_angles[0] = -2.f;

	offset = { 0, 8,  ent.g.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;
}

void Weapon_Shotgun(entity &ent)
{
	Weapon_Generic(ent, 7, 18, 36, 39, { 22, 28, 34 }, { 8, 9 }, weapon_shotgun_fire);
}

static void weapon_supershotgun_fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	offset;
	vector	v;
	int32_t	damage = 6;
	int32_t	kick = 12;

	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	ent.client->g.kick_origin = forward * -2;
	ent.client->g.kick_angles[0] = -2.f;

	offset = { 0, 8,  ent.g.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	v[PITCH] = ent.client->g.v_angle[PITCH];
	v[YAW]   = ent.client->g.v_angle[YAW] - 5;
	v[ROLL]  = ent.client->g.v_angle[ROLL];
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW]   = ent.client->g.v_angle[YAW] + 5;
	AngleVectors(v, &forward, nullptr, nullptr);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index] -= 2;
}

void Weapon_SuperShotgun(entity &ent)
{
	Weapon_Generic(ent, 6, 17, 57, 61, { 29, 42, 57 }, { 7 }, weapon_supershotgun_fire);
}

/*
======================================================================

RAILGUN

======================================================================
*/

static void weapon_railgun_fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	int32_t	damage;
	int32_t	kick;

	if (deathmatch.intVal)
	{
		// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else
	{
		damage = 150;
		kick = 250;
	}
#else
	int32_t damage = 100;
	int32_t kick = 200;
#endif

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vector forward, right;
	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	ent.client->g.kick_origin = forward * -3;
	ent.client->g.kick_angles[0] = -3.f;

	vector offset = { 0, 7, ent.g.viewheight - 8.f };
	vector start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);
	fire_rail(ent, start, forward, damage, kick);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t)ent.s.number);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER
	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index]--;
}

void Weapon_Railgun(entity &ent)
{
	Weapon_Generic(ent, 3, 18, 56, 61, { 56 }, { 4 }, weapon_railgun_fire);
}

/*
======================================================================

BFG10K

======================================================================
*/

static void weapon_bfg_fire(entity &ent)
{
	vector	offset, start;
	vector	forward, right;
	float	damage_radius = 1000.f;
#ifdef SINGLE_PLAYER
	int32_t	damage;

	if !(deathmatch.intVal)
		damage = 500;
	else
		damage = 200;
#else
	int32_t	damage = 200;
#endif

	AngleVectors(ent.client->g.v_angle, &forward, &right, nullptr);

	offset = { 8, 8, ent.g.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	if (ent.client->ps.gunframe == 9)
	{
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort((int16_t)ent.s.number);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent.s.origin, MULTICAST_PVS);

		ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

		PlayerNoise(ent, start, PNOISE_WEAPON);
#endif
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent.client->g.pers.inventory[ent.client->g.ammo_index] < 50)
	{
		ent.client->ps.gunframe++;
		return;
	}

	ent.client->g.kick_origin = forward * -2;

	// make a big pitch kick with an inverse fall
	ent.client->g.v_dmg_pitch = -40.f;
	ent.client->g.v_dmg_roll = random(-8.f, 8.f);
	ent.client->g.v_dmg_time = level.time + DAMAGE_TIME;

	if (is_quad)
		damage *= damage_multiplier;
	fire_bfg(ent, start, forward, damage, 400, damage_radius);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags)dmflags & DF_INFINITE_AMMO))
		ent.client->g.pers.inventory[ent.client->g.ammo_index] -= 50;
}

void Weapon_BFG(entity &ent)
{
	Weapon_Generic(ent, 8, 32, 55, 58, { 39, 45, 50, 55 }, { 9, 17 }, weapon_bfg_fire);
}

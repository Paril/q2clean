#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "lib/math/random.h"
#include "game/player_frames.h"
#include "chainfist.h"
#include "game/rogue/ballistics/melee.h"

constexpr means_of_death MOD_CHAINFIST { .other_kill_fmt = "{0} was shredded by {3}'s ripsaw.\n" };

constexpr int32_t CHAINFIST_REACH = 64;

static void weapon_chainfist_fire(entity &ent)
{
#ifdef SINGLE_PLAYER
	const int32_t damage = (!deathmatch ? 15 : 30) * damage_multiplier;
#else
	constexpr int32_t damage = 30 * damage_multiplier;
#endif

	vector forward, right, up;
	AngleVectors (ent.client.v_angle, &forward, &right, &up);

	// kick back
	ent.client.kick_origin = forward * -2;
	ent.client.kick_angles[0] = -1.f;

	// set start point
	vector offset { 0, 8, ent.viewheight - 4.f };
	vector start = P_ProjectSource (ent, ent.origin, offset, forward, right);

	fire_player_melee (ent, start, forward, CHAINFIST_REACH, damage, 100, MOD_CHAINFIST);
#if defined(SINGLE_PLAYER)

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	ent.client.ps.gunframe++;
	if (!(dmflags & DF_INFINITE_AMMO))
		ent.client.pers.inventory[ent.client.ammo_index] -= ent.client.pers.weapon->quantity;
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
static void chainfist_smoke(entity &ent)
{
	vector forward, right, up;
	AngleVectors(ent.client.v_angle, &forward, &right, &up);

	vector offset { 8, 8, ent.viewheight - 4.f };
	vector v = P_ProjectSource (ent, ent.origin, offset, forward, right);

	gi.ConstructMessage(svc_temp_entity, TE_CHAINFIST_SMOKE, v).unicast(ent, false);
}

void Weapon_ChainFist(entity &ent)
{
	if (ent.client.ps.gunframe == 13 || ent.client.ps.gunframe == 23)			// end of attack, go idle
		ent.client.ps.gunframe = 32;
	// holds for idle sequence
	else if (ent.client.ps.gunframe == 42 && (Q_rand() & 7))
	{
		if ((ent.client.pers.hand != CENTER_HANDED) && random() < 0.4)
			chainfist_smoke(ent);
	}
	else if (ent.client.ps.gunframe == 51 && (Q_rand() & 7))
	{
		if ((ent.client.pers.hand != CENTER_HANDED) && random() < 0.4)
			chainfist_smoke(ent);
	}	

	// set the appropriate weapon sound.
	if (ent.client.weaponstate == WEAPON_FIRING)
		ent.client.weapon_sound = gi.soundindex("weapons/sawhit.wav");
	else if (ent.client.weaponstate == WEAPON_DROPPING)
		ent.client.weapon_sound = SOUND_NONE;
	else
		ent.client.weapon_sound = gi.soundindex("weapons/sawidle.wav");

	Weapon_Generic (ent, 4, 32, 57, 60, G_FrameIsNone, G_FrameIsOneOf<8, 9, 16, 17, 18, 30, 31>, weapon_chainfist_fire);

	if (ent.client.buttons & BUTTON_ATTACK)
	{
		if (ent.client.ps.gunframe == 13 ||
			ent.client.ps.gunframe == 23 ||
			ent.client.ps.gunframe == 32)
		{
			ent.client.chainfist_last_sequence = ent.client.ps.gunframe;
			ent.client.ps.gunframe = 6;
		}
	}

	if (ent.client.ps.gunframe == 6)
	{
		float chance = random();

		if (ent.client.chainfist_last_sequence == 13)		// if we just did sequence 1, do 2 or 3.
			chance -= 0.34f;
		else if (ent.client.chainfist_last_sequence == 23)	// if we just did sequence 2, do 1 or 3
			chance += 0.33f;
		else if (ent.client.chainfist_last_sequence == 32)	// if we just did sequence 3, do 1 or 2
		{
			if (chance >= 0.33f)
				chance += 0.34f;
		}

		if (chance < 0.33f)
			ent.client.ps.gunframe = 14;
		else if (chance < 0.66f)
			ent.client.ps.gunframe = 24;
	}
}
#endif
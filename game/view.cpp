#include "config.h"
#include "lib/gi.h"
#include "game.h"
#include "hud.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "items/armor.h"
#include "combat.h"
#include "weaponry.h"
#include "player_frames.h"
#include "view.h"

constexpr gtimef FALL_TIME = 0.3s;

static vector forward, right, up;
static float xyspeed, bobmove, bobfracsin;
static int32_t bobcycle;

/*
=============
P_WorldEffects
=============
*/
static inline void P_WorldEffects(entity &current_player)
{
	bool	breather;
	bool	envirosuit;
	int32_t	current_waterlevel, old_waterlevel;

	if (current_player.movetype == MOVETYPE_NOCLIP)
	{
		current_player.air_finished_framenum = level.framenum + 12s; // don't need air
		return;
	}

	current_waterlevel = current_player.waterlevel;
	old_waterlevel = current_player.client->old_waterlevel;
	current_player.client->old_waterlevel = current_waterlevel;

	breather = current_player.client->breather_framenum > level.framenum;
	envirosuit = current_player.client->enviro_framenum > level.framenum;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && current_waterlevel)
	{
#ifdef SINGLE_PLAYER
		PlayerNoise(current_player, current_player.origin, PNOISE_SELF);
#endif

		if (current_player.watertype & CONTENTS_LAVA)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"));
		else if (current_player.watertype & CONTENTS_SLIME)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"));
		else if (current_player.watertype & CONTENTS_WATER)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"));
		current_player.flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player.damage_debounce_framenum = level.framenum - 1s;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && !current_waterlevel)
	{
#ifdef SINGLE_PLAYER
		PlayerNoise(current_player, current_player.origin, PNOISE_SELF);
#endif
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"));
		current_player.flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && current_waterlevel == 3)
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"));

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && current_waterlevel != 3)
	{
		if (current_player.air_finished_framenum < level.framenum)
#ifdef SINGLE_PLAYER
		{
#endif
			// gasp for air
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"));
#ifdef SINGLE_PLAYER
			PlayerNoise(current_player, current_player.origin, PNOISE_SELF);
		}
#endif
		else if (current_player.air_finished_framenum < level.framenum + 11s)
			// just break surface
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"));
	}

	//
	// check for drowning
	//
	if (current_waterlevel == 3)
	{
		// breather or envirosuit give air
		if (breather || envirosuit)
		{
			current_player.air_finished_framenum = level.framenum + 10s;

			if (((current_player.client->breather_framenum - level.framenum) % 2500ms) == gtimef::zero())
			{
				if (!current_player.client->breather_sound)
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"));
				else
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"));

				current_player.client->breather_sound = !current_player.client->breather_sound;
#ifdef SINGLE_PLAYER
				PlayerNoise(current_player, current_player.origin, PNOISE_SELF);
#endif
			}
		}

		// if out of air, start drowning
		if (current_player.air_finished_framenum < level.framenum)
		{
			// drown!
			if (current_player.client->next_drown_framenum < level.framenum && current_player.health > 0)
			{
				current_player.client->next_drown_framenum = level.framenum + 1s;

				// take more damage the longer underwater
				current_player.dmg += 2;
				if (current_player.dmg > 15)
					current_player.dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player.health <= current_player.dmg)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"));
				else if (Q_rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"));
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"));

				current_player.pain_debounce_framenum = level.framenum;

				T_Damage(current_player, world, world, vec3_origin, current_player.origin, vec3_origin, current_player.dmg, 0, { DAMAGE_NO_ARMOR }, MOD_WATER);
			}
		}
	}
	else
	{
		current_player.air_finished_framenum = level.framenum + 12s;
		current_player.dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (current_waterlevel && (current_player.watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		if (current_player.watertype & CONTENTS_LAVA)
		{
			if (current_player.health > 0
				&& current_player.pain_debounce_framenum <= level.framenum
				&& current_player.client->invincible_framenum < level.framenum)
			{
				if (Q_rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"));
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"));
				current_player.pain_debounce_framenum = level.framenum + 1s;
			}

			if (envirosuit) // take 1/3 damage with envirosuit
				T_Damage(current_player, world, world, vec3_origin, current_player.origin, vec3_origin, 1 * current_waterlevel, 0, { DAMAGE_NONE }, MOD_LAVA);
			else
				T_Damage(current_player, world, world, vec3_origin, current_player.origin, vec3_origin, 3 * current_waterlevel, 0, { DAMAGE_NONE }, MOD_LAVA);
		}

		// no damage from slime with envirosuit
		if ((current_player.watertype & CONTENTS_SLIME) && !envirosuit)
			T_Damage(current_player, world, world, vec3_origin, current_player.origin, vec3_origin, 1 * current_waterlevel, 0, { DAMAGE_NONE }, MOD_SLIME);
	}
}

/*
===============
SV_CalcRoll
===============
*/
static inline float SV_CalcRoll(vector velocity)
{
	float side = velocity * right;
	float sign = side < 0.f ? -1.f : 1.f;
	side = fabs(side);
	const float &value = sv_rollangle.value;

	if (side < sv_rollspeed)
		side = side * value / sv_rollspeed;
	else
		side = value;

	return side * sign;
}

constexpr means_of_death MOD_FALLING { .self_kill_fmt = "{0} cratered.\n" };

/*
=================
P_FallingDamage
=================
*/
static inline void P_FallingDamage(entity &ent)
{
	float	delta;
	int32_t	damage;

	if (ent.modelindex != MODEL_PLAYER)
		return;     // not in the player model

	if (ent.movetype == MOVETYPE_NOCLIP)
		return;

	if ((ent.client->oldvelocity[2] < 0.f) && (ent.velocity[2] > ent.client->oldvelocity[2]) && !ent.groundentity.has_value())
		delta = ent.client->oldvelocity[2];
	else if (!ent.groundentity.has_value())
		return;
	else
		delta = ent.velocity.z - ent.client->oldvelocity[2];

	delta = delta * delta * 0.0001f;

#ifdef HOOK_CODE
	// never take damage if just release grapple or on grapple
	if ((level.framenum - ent.client->grapplereleaseframenum) <= 2s ||
		(ent.client->grapple.has_value() &&
			ent.client->grapplestate > GRAPPLE_STATE_FLY))
		return;
#endif

	// never take falling damage if completely underwater
	if (ent.waterlevel == WATER_UNDER)
		return;
	else if (ent.waterlevel == WATER_WAIST)
		delta *= 0.25f;
	else if (ent.waterlevel == WATER_LEGS)
		delta *= 0.5f;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		ent.event = EV_FOOTSTEP;
		return;
	}

	ent.client->fall_value = min(40.f, delta * 0.5f);
	ent.client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent.health > 0)
		{
			if (delta >= 55)
				ent.event = EV_FALLFAR;
			else
				ent.event = EV_FALL;
		}
		ent.pain_debounce_framenum = level.framenum;   // no normal pain sound
		damage = (int32_t) ((delta - 30) / 2);
		if (damage < 1)
			damage = 1;

		if (!(dmflags & DF_NO_FALLING))
			T_Damage(ent, world, world, MOVEDIR_UP, ent.origin, vec3_origin, damage, 0, { DAMAGE_NONE }, MOD_FALLING);
	}
	else
	{
		ent.event = EV_FALLSHORT;
		return;
	}
}

constexpr vector power_color = { 0.0, 1.0, 0.0 };
constexpr vector acolor = { 1.0, 1.0, 1.0 };
constexpr vector bcolor = { 1.0, 0.0, 0.0 };

/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
static void P_DamageFeedback(entity &player)
{
	float	side;
	float	realcount, dcount, kick;
	int32_t	r, l;

	// flash the backgrounds behind the status numbers
	player.client->ps.stats[STAT_FLASHES] = 0;
	if (player.client->damage_blood)
		player.client->ps.stats[STAT_FLASHES] |= 1;
	if (player.client->damage_armor && !(player.flags & FL_GODMODE) && (player.client->invincible_framenum <= level.framenum))
		player.client->ps.stats[STAT_FLASHES] |= 2;

	// total points of damage shot at the player this frame
	dcount = (float) (player.client->damage_blood + player.client->damage_armor + player.client->damage_parmor);
	if (!dcount)
		return;     // didn't take any damage

	// start a pain animation if still in the player model
	if (player.client->anim_priority < ANIM_PAIN && player.modelindex == MODEL_PLAYER)
	{
		static int32_t i;

		player.client->anim_priority = ANIM_PAIN;
		if (player.client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player.frame = FRAME_crpain1 - 1;
			player.client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i + 1) % 3;
			switch (i) {
			case 0:
				player.frame = FRAME_pain101 - 1;
				player.client->anim_end = FRAME_pain104;
				break;
			case 1:
				player.frame = FRAME_pain201 - 1;
				player.client->anim_end = FRAME_pain204;
				break;
			case 2:
				player.frame = FRAME_pain301 - 1;
				player.client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	realcount = dcount;
	if (dcount < 10)
		dcount = 10.f; // always make a visible effect

	// play an apropriate pain sound
	if ((level.framenum > player.pain_debounce_framenum) && !(player.flags & FL_GODMODE) && (player.client->invincible_framenum <= level.framenum))
	{
		r = 1 + (Q_rand() & 1);
		player.pain_debounce_framenum = level.framenum + 700ms;
		if (player.health < 25)
			l = 25;
		else if (player.health < 50)
			l = 50;
		else if (player.health < 75)
			l = 75;
		else
			l = 100;
		gi.sound(player, CHAN_VOICE, gi.soundindexfmt("*pain{}_{}.wav", l, r));
	}

	// the total alpha of the blend is always proportional to count
	if (player.client->damage_alpha < 0)
		player.client->damage_alpha = 0;
	player.client->damage_alpha += dcount * 0.01f;
	if (player.client->damage_alpha < 0.2f)
		player.client->damage_alpha = 0.2f;
	if (player.client->damage_alpha > 0.6f)
		player.client->damage_alpha = 0.6f;    // don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	vector blend = vec3_origin;
	if (player.client->damage_parmor)
		blend += ((float) player.client->damage_parmor / realcount * power_color);
	if (player.client->damage_armor)
		blend += ((float) player.client->damage_armor / realcount * acolor);
	if (player.client->damage_blood)
		blend += ((float) player.client->damage_blood / realcount * bcolor);
	player.client->damage_blend = blend;


	//
	// calculate view angle kicks
	//
	kick = (float) fabs(player.client->damage_knockback);
	// kick of 0 means no view adjust at all
	if (kick && player.health > 0)
	{
		kick = kick * 100 / player.health;

		if (kick < dcount * 0.5f)
			kick = dcount * 0.5f;
		if (kick > 50)
			kick = 50.f;

		vector v = player.client->damage_from - player.origin;
		VectorNormalize(v);

		side = v * right;
		player.client->v_dmg_roll = kick * side * 0.3f;

		side = -(v * forward);
		player.client->v_dmg_pitch = kick * side * 0.3f;

		player.client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	player.client->damage_blood = 0;
	player.client->damage_armor = 0;
	player.client->damage_parmor = 0;
	player.client->damage_knockback = 0;
}

/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
static inline void SV_CalcViewOffset(entity &ent)
{
	float	bob;
	float	ratio;
	float	delta;
	vector	v;

	//===================================

		// if dead, fix the angle and don't add any kick
	if (ent.deadflag)
	{
		ent.client->ps.kick_angles = vec3_origin;

		ent.client->ps.viewangles[ROLL] = 40.f;
		ent.client->ps.viewangles[PITCH] = -15.f;
		ent.client->ps.viewangles[YAW] = ent.client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick

		ent.client->ps.kick_angles = ent.client->kick_angles;

		// add angles based on damage kick

		ratio = (ent.client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent.client->v_dmg_pitch = 0;
			ent.client->v_dmg_roll = 0;
		}
		ent.client->ps.kick_angles[PITCH] += ratio * ent.client->v_dmg_pitch;
		ent.client->ps.kick_angles[ROLL] += ratio * ent.client->v_dmg_roll;

		// add pitch based on fall kick

		ratio = (ent.client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		ent.client->ps.kick_angles[PITCH] += ratio * ent.client->fall_value;

		// add angles based on velocity

		delta = ent.velocity * forward;
		ent.client->ps.kick_angles[PITCH] += delta * run_pitch;

		delta = ent.velocity * right;
		ent.client->ps.kick_angles[ROLL] += delta * run_roll;

		// add angles based on bob

		delta = bobfracsin * bob_pitch * xyspeed;
		if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;     // crouching
		ent.client->ps.kick_angles[PITCH] += delta;
		delta = bobfracsin * bob_roll * xyspeed;
		if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;     // crouching
		if (bobcycle & 1)
			delta = -delta;
		ent.client->ps.kick_angles[ROLL] += delta;
	}

	//===================================

		// base origin

	v = vec3_origin;

	// add view height

	v.z += ent.viewheight;

	// add fall height

	ratio = (ent.client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v.z -= ratio * ent.client->fall_value * 0.4f;

	// add bob height

	bob = bobfracsin * xyspeed * (float) bob_up;
	if (bob > 6)
		bob = 6.f;

	v.z += bob;

	// add kick offset
	v += ent.client->kick_origin;

	// absolutely bound offsets
	// so the view can never be outside the player box
	if (v.x < -14)
		v.x = -14.f;
	else if (v.x > 14)
		v.x = 14.f;
	if (v.y < -14)
		v.y = -14.f;
	else if (v.y > 14)
		v.y = 14.f;
	if (v.z < -22)
		v.z = -22.f;
	else if (v.z > 30)
		v.z = 30.f;

	ent.client->ps.viewoffset = v;
}

/*
==============
SV_CalcGunOffset
==============
*/
static inline void SV_CalcGunOffset(entity &ent)
{
	// gun angles from bobbing
#ifdef GROUND_ZERO
	//ROGUE - heatbeam shouldn't bob so the beam looks right
	if (ent.client->pers.weapon && ent.client->pers.weapon->id != ITEM_PLASMA_BEAM)
	{
#endif
		ent.client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005f;
		ent.client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01f;

		if (bobcycle & 1)
		{
			ent.client->ps.gunangles[ROLL] = -ent.client->ps.gunangles[ROLL];
			ent.client->ps.gunangles[YAW] = -ent.client->ps.gunangles[YAW];
		}

		ent.client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005f;

		// gun angles from delta movement
		for (int32_t i = 0; i < 3; i++)
		{
			float delta = ent.client->oldviewangles[i] - ent.client->ps.viewangles[i];
			if (delta > 180)
				delta -= 360.f;
			if (delta < -180)
				delta += 360.f;
			if (delta > 45)
				delta = 45.f;
			if (delta < -45)
				delta = -45.f;
			if (i == YAW)
				ent.client->ps.gunangles[ROLL] += 0.1f * delta;
			ent.client->ps.gunangles[i] += 0.2f * delta;
		}
#ifdef GROUND_ZERO
	}
	else
		ent.client->ps.gunangles = vec3_origin;
#endif

	// gun height
	ent.client->ps.gunoffset = vec3_origin;
}

/*
=============
SV_AddBlend
=============
*/
static inline void SV_AddBlend(vector rgb, float a, array<float, 4> &v_blend)
{
	float a2 = v_blend[3] + (1 - v_blend[3]) * a; // new total alpha
	float a3 = v_blend[3] / a2;   // fraction of color from old

	((vector &) v_blend) = (((vector &) v_blend) * a3) + (rgb * (1 - a3));

	v_blend[3] = a2;
}

constexpr vector lava_blend = { 1.0, 0.3f, 0.0 };
constexpr vector bonus_blend = { 0.85f, 0.7f, 0.3f };
constexpr vector slime_blend = { 0.0, 0.1f, 0.05f };
constexpr vector water_blend = { 0.5f, 0.3f, 0.2f };
constexpr vector quad_blend = { 0, 0, 1 };
constexpr vector invul_blend = { 1, 1, 0 };
constexpr vector enviro_blend = { 0, 1, 0 };
constexpr vector breather_blend = { 0.4f, 1, 0.4f };

#ifdef THE_RECKONING
constexpr vector quadfire_blend = { 1, 0.2f, 0.5f };
#endif

#ifdef GROUND_ZERO
constexpr vector double_blend = { 0.9f, 0.7f, 0 };
constexpr vector nuke_blend = { 1, 1, 1 };
constexpr vector ir_blend = { 1, 0, 0 };
#endif

/*
=============
SV_CalcBlend
=============
*/
static inline void SV_CalcBlend(entity &ent)
{
	content_flags	contents;
	vector			vieworg;
	gtime			remaining;

	ent.client->ps.blend = { 0, 0, 0, 0 };

	// add for contents
	vieworg = ent.origin + ent.client->ps.viewoffset;
	contents = gi.pointcontents(vieworg);
	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
		ent.client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent.client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
		SV_AddBlend(lava_blend, 0.6f, ent.client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend(slime_blend, 0.6f, ent.client->ps.blend);
	else if (contents & CONTENTS_WATER)
		SV_AddBlend(water_blend, 0.4f, ent.client->ps.blend);

	// add for powerups
	if (ent.client->quad_framenum > level.framenum)
	{
		remaining = ent.client->quad_framenum - level.framenum;
		if (remaining == 3s)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(quad_blend, 0.08f, ent.client->ps.blend);
	}
#ifdef THE_RECKONING
	// RAFAEL
	else if (ent.client->quadfire_framenum > level.framenum)
	{
		remaining = ent.client->quadfire_framenum - level.framenum;
		if (remaining == 3s)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire2.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(quadfire_blend, 0.08f, ent.client->ps.blend);
	}
#endif
#ifdef GROUND_ZERO
	else if (ent.client->double_framenum > level.framenum)
	{
		remaining = ent.client->double_framenum - level.framenum;
		if (remaining == 3s)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage2.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(double_blend, 0.08f, ent.client->ps.blend);
	}
#endif
	else if (ent.client->invincible_framenum > level.framenum)
	{
		remaining = ent.client->invincible_framenum - level.framenum;
		if (remaining == 3s)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(invul_blend, 0.08f, ent.client->ps.blend);
	}
	else if (ent.client->enviro_framenum > level.framenum)
	{
		remaining = ent.client->enviro_framenum - level.framenum;
		if (remaining == 3s)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(enviro_blend, 0.08f, ent.client->ps.blend);
	}
	else if (ent.client->breather_framenum > level.framenum)
	{
		remaining = ent.client->breather_framenum - level.framenum;
		if (remaining == 3s)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"));
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			SV_AddBlend(breather_blend, 0.04f, ent.client->ps.blend);
	}

#ifdef GROUND_ZERO
	if (ent.client->nuke_framenum > level.framenum)
	{
		float brightness = (ent.client->nuke_framenum - level.framenum) / 2s;
		SV_AddBlend(nuke_blend, brightness, ent.client->ps.blend);
	}

	if (ent.client->ir_framenum > level.framenum)
	{
		remaining = ent.client->ir_framenum - level.framenum;
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
		{
			ent.client->ps.rdflags |= RDF_IRGOGGLES;
			SV_AddBlend(ir_blend, 0.2f, ent.client->ps.blend);
		}
		else
			ent.client->ps.rdflags &= ~RDF_IRGOGGLES;
	}
	else
		ent.client->ps.rdflags &= ~RDF_IRGOGGLES;
#endif

	// add for damage
	if (ent.client->damage_alpha > 0)
		SV_AddBlend(ent.client->damage_blend, ent.client->damage_alpha, ent.client->ps.blend);

	if (ent.client->bonus_alpha > 0)
		SV_AddBlend(bonus_blend, ent.client->bonus_alpha, ent.client->ps.blend);

	// drop the damage value
	ent.client->damage_alpha -= 0.06f;
	if (ent.client->damage_alpha < 0)
		ent.client->damage_alpha = 0;

	// drop the bonus value
	ent.client->bonus_alpha -= 0.1f;
	if (ent.client->bonus_alpha < 0)
		ent.client->bonus_alpha = 0;
}

/*
===============
G_SetClientEvent
===============
*/
static inline void G_SetClientEvent(entity &ent)
{
	if (ent.event)
		return;

	if (ent.groundentity.has_value() && xyspeed > 225)
		if ((int32_t) (ent.client->bobtime + bobmove) != bobcycle)
			ent.event = EV_FOOTSTEP;
}

/*
===============
G_SetClientEffects
===============
*/
static inline void G_SetClientEffects(entity &ent)
{
	gtime	remaining;

	ent.effects = EF_NONE;
	ent.renderfx = RF_IR_VISIBLE;

	if (ent.health <= 0 || level.intermission_framenum != gtime::zero())
		return;

	if (ent.powerarmor_framenum > level.framenum)
	{
		gitem_id pa_type = PowerArmorType(ent);
		if (pa_type == ITEM_POWER_SCREEN)
			ent.effects |= EF_POWERSCREEN;
		else if (pa_type == ITEM_POWER_SHIELD)
		{
			ent.effects |= EF_COLOR_SHELL;
			ent.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent.client->quad_framenum > level.framenum)
	{
		remaining = ent.client->quad_framenum - level.framenum;
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			ent.effects |= EF_QUAD;
	}

#ifdef THE_RECKONING
	// RAFAEL
	if (ent.client->quadfire_framenum > level.framenum)
	{
		remaining = ent.client->quadfire_framenum - level.framenum;
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			ent.effects |= EF_QUAD;
	}
#endif

#ifdef GROUND_ZERO
	if (ent.client->double_framenum > level.framenum)
	{
		remaining = ent.client->double_framenum - level.framenum;
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			ent.effects |= EF_DOUBLE;
	}

	if (ent.client->tracker_pain_framenum > level.framenum)
		ent.effects |= EF_TRACKERTRAIL;
#endif

	if (ent.client->invincible_framenum > level.framenum)
	{
		remaining = ent.client->invincible_framenum - level.framenum;
		if (remaining > 3s || (remaining % 800ms) >= 400ms)
			ent.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent.flags & FL_GODMODE)
	{
		ent.effects |= EF_COLOR_SHELL;
		ent.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}

#ifdef CTF
	CTFEffects(ent);
#endif
}

/*
===============
G_SetClientSound
===============
*/
static inline void G_SetClientSound(entity &ent)
{
#ifdef SINGLE_PLAYER
	if (ent.client->pers.game_helpchanged != game.helpchanged)
	{
		ent.client->pers.game_helpchanged = game.helpchanged;
		ent.client->pers.helpchanged = 1;
	}

	// help beep (no more than three times)
	if (ent.client->pers.helpchanged && ent.client->pers.helpchanged <= 3 && (level.framenum % 6400) == gtime::zero())
	{
		ent.client->pers.helpchanged++;
		gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/pc_up.wav"), ATTN_STATIC);
	}
#endif

	ent.sound = SOUND_NONE;

	if (ent.waterlevel && (ent.watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
		ent.sound = snd_fry;
	else if (ent.client->weapon_sound)
		ent.sound = ent.client->weapon_sound;
	else if (ent.client->pers.weapon)
	{
		gitem_id weap = ent.client->pers.weapon->id;

		if (weap == ITEM_RAILGUN)
			ent.sound = gi.soundindex("weapons/rg_hum.wav");
		else if (weap == ITEM_BFG)
			ent.sound = gi.soundindex("weapons/bfg_hum.wav");
#ifdef THE_RECKONING
		else if (weap == ITEM_PHALANX)
			ent.sound = gi.soundindex("weapons/phaloop.wav");
#endif
	}
}

/*
===============
G_SetClientFrame
===============
*/
static inline void G_SetClientFrame(entity &ent)
{
	if (ent.modelindex != MODEL_PLAYER)
		return;     // not in the player model

	const bool duck = ent.client->ps.pmove.pm_flags & PMF_DUCKED;
	const bool run = xyspeed;

	// check for stand/duck and stop/go transitions
	if (duck != ent.client->anim_duck && ent.client->anim_priority < ANIM_DEATH)
		goto newanim;
	if (run != ent.client->anim_run && ent.client->anim_priority == ANIM_BASIC)
		goto newanim;
	if (!ent.groundentity.has_value() && ent.client->anim_priority <= ANIM_WAVE)
		goto newanim;

	if (ent.client->anim_priority == ANIM_REVERSE)
	{
		if ((size_t) ent.frame > ent.client->anim_end)
		{
			ent.frame--;
			return;
		}
	}
	else if ((size_t) ent.frame < ent.client->anim_end)
	{
		// continue an animation
		ent.frame++;
		return;
	}

	if (ent.client->anim_priority == ANIM_DEATH)
		return;     // stay there
	if (ent.client->anim_priority == ANIM_JUMP)
	{
		if (!ent.groundentity.has_value())
			return;     // stay there
		ent.client->anim_priority = ANIM_WAVE;
		ent.frame = FRAME_jump3;
		ent.client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	ent.client->anim_priority = ANIM_BASIC;
	ent.client->anim_duck = duck;
	ent.client->anim_run = run;

	if (!ent.groundentity.has_value())
	{
#ifdef HOOK_CODE
		// if on grapple, don't go into jump frame, go into standing frame
		if (ent.client->grapple.has_value())
		{
			ent.frame = FRAME_stand01;
			ent.client->anim_end = FRAME_stand40;
		}
		else
		{
#endif
			ent.client->anim_priority = ANIM_JUMP;
			if (ent.frame != FRAME_jump2)
				ent.frame = FRAME_jump1;
			ent.client->anim_end = FRAME_jump2;
#ifdef HOOK_CODE
		}
#endif
	}
	else if (run)
	{
		// running
		if (duck)
		{
			ent.frame = FRAME_crwalk1;
			ent.client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent.frame = FRAME_run1;
			ent.client->anim_end = FRAME_run6;
		}
	}
	else
	{
		// standing
		if (duck)
		{
			ent.frame = FRAME_crstnd01;
			ent.client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent.frame = FRAME_stand01;
			ent.client->anim_end = FRAME_stand40;
		}
	}
}

void ClientEndServerFrame(entity &ent)
{
	float   bobtime;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	ent.client->ps.pmove.set_origin(ent.origin);
	ent.client->ps.pmove.set_velocity(ent.velocity);

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermission_framenum != gtime::zero())
	{
		// FIXME: add view drifting here?
		ent.client->ps.blend[3] = 0.f;
		ent.client->ps.fov = 90.f;
		G_SetStats(ent);
		return;
	}

	AngleVectors(ent.client->v_angle, &forward, &right, &up);

	// burn from lava, etc
	P_WorldEffects(ent);

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent.client->v_angle[PITCH] > 180)
		ent.angles[PITCH] = (-360 + ent.client->v_angle[PITCH]) / 3;
	else
		ent.angles[PITCH] = ent.client->v_angle[PITCH] / 3;
	ent.angles[YAW] = ent.client->v_angle[YAW];
	ent.angles[ROLL] = 0;
	ent.angles[ROLL] = SV_CalcRoll(ent.velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrt(ent.velocity.x * ent.velocity.x + ent.velocity.y * ent.velocity.y);

	if (xyspeed < 5)
	{
		bobmove = 0;
		ent.client->bobtime = 0;    // start at beginning of cycle again
	}
	else if (ent.groundentity.has_value())
	{
		// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25f;
		else if (xyspeed > 100)
			bobmove = 0.125f;
		else
			bobmove = 0.0625f;
	}

	bobtime = (ent.client->bobtime += bobmove);

	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4;

	bobcycle = (int) bobtime;
	bobfracsin = fabs(sin(bobtime * PI));

	// detect hitting the floor
	P_FallingDamage(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// determine the view offsets
	SV_CalcViewOffset(ent);

	// determine the gun offsets
	SV_CalcGunOffset(ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend(ent);

	// chase cam stuff
	if (ent.client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats(ent);

	G_CheckChaseStats(ent);

	G_SetClientEvent(ent);

	G_SetClientEffects(ent);

	G_SetClientSound(ent);

	G_SetClientFrame(ent);

	ent.client->oldvelocity = ent.velocity;
	ent.client->oldviewangles = ent.client->ps.viewangles;

	// clear weapon kicks
	ent.client->kick_origin = ent.client->kick_angles = vec3_origin;

	// if the scoreboard is up, update it
	if (ent.client->showscores && (level.framenum % 3200) == gtime::zero())
#ifdef PMENU
		if (!ent.client.menu.open)
#endif
			DeathmatchScoreboardMessage(ent, ent.enemy, false);
}

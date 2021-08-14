#include "config.h"
#include "entity.h"
#include "func.h"
#include "game.h"
#include "spawn.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/combat.h"
#include "game/misc.h"

/*
=========================================================

  PLATS

  movement options:

  linear
  smooth start, hard stop
  smooth start, smooth stop

  start
  end
  acceleration
  speed
  deceleration
  begin sound
  end sound
  target fired when reaching end
  wait at end

  object characteristics that use move segments
  ---------------------------------------------
  movetype_push, or movetype_stop
  action when touched
  action when blocked
  action when used
	disabled?
  auto trigger spawning


=========================================================
*/

constexpr spawn_flag DOOR_START_OPEN	= (spawn_flag)1;
constexpr spawn_flag DOOR_REVERSE		= (spawn_flag)2;
constexpr spawn_flag DOOR_CRUSHER		= (spawn_flag)4;
constexpr spawn_flag DOOR_NOMONSTER		= (spawn_flag)8;
constexpr spawn_flag DOOR_TOGGLE		= (spawn_flag)32;
constexpr spawn_flag DOOR_X_AXIS		= (spawn_flag)64;
constexpr spawn_flag DOOR_Y_AXIS		= (spawn_flag)128;
#ifdef GROUND_ZERO
constexpr spawn_flag DOOR_INACTIVE		= (spawn_flag)8192;
#endif

//
// Support routines for movement (changes in origin using velocity)
//

static void Move_Done(entity &ent)
{
	ent.velocity = vec3_origin;
	ent.moveinfo.endfunc(ent);
}

REGISTER_STATIC_SAVABLE(Move_Done);

static void Move_Final(entity &ent)
{
	if (ent.moveinfo.remaining_distance == 0)
	{
		Move_Done(ent);
		return;
	}

	ent.velocity = ent.moveinfo.dir * (ent.moveinfo.remaining_distance / FRAMETIME.count());

	ent.think = SAVABLE(Move_Done);
	ent.nextthink = level.framenum + 1_hz;
}

REGISTER_STATIC_SAVABLE(Move_Final);

static void Move_Begin(entity &ent)
{
	if ((ent.moveinfo.speed * FRAMETIME.count()) >= ent.moveinfo.remaining_distance)
	{
		Move_Final(ent);
		return;
	}

	ent.velocity = ent.moveinfo.dir * ent.moveinfo.speed;
	float num_frames = floor((ent.moveinfo.remaining_distance / ent.moveinfo.speed) / FRAMETIME.count());
	ent.moveinfo.remaining_distance -= num_frames * ent.moveinfo.speed * FRAMETIME.count();
	ent.nextthink = level.framenum + frames((int32_t) num_frames);
	ent.think = SAVABLE(Move_Final);
}

REGISTER_STATIC_SAVABLE(Move_Begin);

static void Think_AccelMove(entity &ent);

REGISTER_STATIC_SAVABLE(Think_AccelMove);

static void plat_CalcAcceleratedMove(entity &ent);

void Move_Calc(entity &ent, vector dest, savable<ethinkfunc> func)
{
	ent.velocity = vec3_origin;
	ent.moveinfo.dir = dest - ent.origin;
	ent.moveinfo.remaining_distance = VectorNormalize(ent.moveinfo.dir);
	ent.moveinfo.endfunc = func;

	if (ent.moveinfo.speed == ent.moveinfo.accel && ent.moveinfo.speed == ent.moveinfo.decel)
	{
		if (level.current_entity == ((ent.flags & FL_TEAMSLAVE) ? (entity &)ent.teammaster : ent))
			Move_Begin(ent);
		else
		{
			ent.nextthink = level.framenum + 1_hz;
			ent.think = SAVABLE(Move_Begin);
		}
	}
	else
	{
		// accelerative
		plat_CalcAcceleratedMove(ent);
		
		ent.moveinfo.current_speed = 0;
		ent.think = SAVABLE(Think_AccelMove);
		ent.nextthink = level.framenum + 1_hz;
	}
}

//
// Support routines for angular movement (changes in angle using avelocity)
//

static void AngleMove_Done(entity &ent)
{
	ent.avelocity = vec3_origin;
	ent.moveinfo.endfunc(ent);
}

REGISTER_STATIC_SAVABLE(AngleMove_Done);

static void AngleMove_Final(entity &ent)
{
	vector	move;

	if (ent.moveinfo.state == STATE_UP)
		move = ent.moveinfo.end_angles - ent.angles;
	else
		move = ent.moveinfo.start_angles - ent.angles;

	if (!move)
	{
		AngleMove_Done(ent);
		return;
	}

	ent.avelocity = move * BASE_FRAMERATE;

	ent.think = SAVABLE(AngleMove_Done);
	ent.nextthink = level.framenum + 1_hz;
}

REGISTER_STATIC_SAVABLE(AngleMove_Final);

static void AngleMove_Begin(entity &ent);

REGISTER_STATIC_SAVABLE(AngleMove_Begin);

static void AngleMove_Begin(entity &ent)
{
	vector	destdelta;
	float	len;
	float	traveltime;

#ifdef GROUND_ZERO
	// accelerate as needed
	if (ent.moveinfo.speed < ent.speed)
	{
		ent.moveinfo.speed += ent.accel;
		if(ent.moveinfo.speed > ent.speed)
			ent.moveinfo.speed = ent.speed;
	}
#endif

	// set destdelta to the vector needed to move
	if (ent.moveinfo.state == STATE_UP)
		destdelta = ent.moveinfo.end_angles - ent.angles;
	else
		destdelta = ent.moveinfo.start_angles - ent.angles;

	// calculate length of vector
	len = VectorLength(destdelta);

	// divide by speed to get time to reach dest
	traveltime = len / ent.moveinfo.speed;

	if (traveltime < FRAMETIME.count())
	{
		AngleMove_Final(ent);
		return;
	}

	// scale the destdelta vector by the time spent traveling to get velocity
	ent.avelocity = destdelta * (1.0f / traveltime);

#ifdef GROUND_ZERO
	// if we're done accelerating, act as a normal rotation
	if (ent.moveinfo.speed >= ent.speed)
	{
#endif
		// set nextthink to trigger a think when dest is reached
		ent.nextthink = level.framenum + frames((int32_t) floor(traveltime / FRAMETIME.count()));
		ent.think = SAVABLE(AngleMove_Final);
#ifdef GROUND_ZERO
	}
	else
	{
		ent.nextthink = level.framenum + 1_hz;
		ent.think = SAVABLE(AngleMove_Begin);
	}
#endif
}

static void AngleMove_Calc(entity &ent, savable<ethinkfunc> func)
{
	ent.avelocity = vec3_origin;
	ent.moveinfo.endfunc = func;
	
#ifdef GROUND_ZERO
	// if we're supposed to accelerate, this will tell anglemove_begin to do so
	if(ent.accel != ent.speed)
		ent.moveinfo.speed = 0;
#endif
	
	if (level.current_entity == ((ent.flags & FL_TEAMSLAVE) ? (entity &)ent.teammaster : ent))
		AngleMove_Begin(ent);
	else
	{
		ent.nextthink = level.framenum + 1_hz;
		ent.think = SAVABLE(AngleMove_Begin);
	}
}

/*
==============
Think_AccelMove

The team has completed a frame of movement, so
change the speed for the next frame
==============
*/

constexpr float AccelerationDistance(float target, float rate) { return (target * ((target / rate) + 1) / 2); }

static void plat_CalcAcceleratedMove(entity &ent)
{
	ent.moveinfo.move_speed = ent.moveinfo.speed;

	if (ent.moveinfo.remaining_distance < ent.moveinfo.accel)
	{
		ent.moveinfo.current_speed = ent.moveinfo.remaining_distance;
		return;
	}

	float accel_dist = AccelerationDistance(ent.moveinfo.speed, ent.moveinfo.accel);
	float decel_dist = AccelerationDistance(ent.moveinfo.speed, ent.moveinfo.decel);

	if ((ent.moveinfo.remaining_distance - accel_dist - decel_dist) < 0)
	{
		float f = (ent.moveinfo.accel + ent.moveinfo.decel) / (ent.moveinfo.accel * ent.moveinfo.decel);
		ent.moveinfo.move_speed = (-2 + sqrt(4 - 4 * f * (-2 * ent.moveinfo.remaining_distance))) / (2 * f);
		decel_dist = AccelerationDistance(ent.moveinfo.move_speed, ent.moveinfo.decel);
	}

	ent.moveinfo.decel_distance = decel_dist;
}

static void plat_Accelerate(entity &ent)
{
	// are we decelerating?
	if (ent.moveinfo.remaining_distance <= ent.moveinfo.decel_distance)
	{
		if (ent.moveinfo.remaining_distance < ent.moveinfo.decel_distance)
		{
			if (ent.moveinfo.next_speed)
			{
				ent.moveinfo.current_speed = ent.moveinfo.next_speed;
				ent.moveinfo.next_speed = 0;
				return;
			}
			if (ent.moveinfo.current_speed > ent.moveinfo.decel)
				ent.moveinfo.current_speed -= ent.moveinfo.decel;
		}
		return;
	}

	// are we at full speed and need to start decelerating during this move?
	if (ent.moveinfo.current_speed == ent.moveinfo.move_speed)
		if ((ent.moveinfo.remaining_distance - ent.moveinfo.current_speed) < ent.moveinfo.decel_distance)
		{
			float p1_distance = ent.moveinfo.remaining_distance - ent.moveinfo.decel_distance;
			float p2_distance = ent.moveinfo.move_speed * (1.0f - (p1_distance / ent.moveinfo.move_speed));
			float distance = p1_distance + p2_distance;
			ent.moveinfo.current_speed = ent.moveinfo.move_speed;
			ent.moveinfo.next_speed = ent.moveinfo.move_speed - ent.moveinfo.decel * (p2_distance / distance);
			return;
		}

	// are we accelerating?
	if (ent.moveinfo.current_speed < ent.moveinfo.speed)
	{
		float old_speed = ent.moveinfo.current_speed;

		// figure simple acceleration up to move_speed
		ent.moveinfo.current_speed += ent.moveinfo.accel;
		if (ent.moveinfo.current_speed > ent.moveinfo.speed)
			ent.moveinfo.current_speed = ent.moveinfo.speed;

		// are we accelerating throughout this entire move?
		if ((ent.moveinfo.remaining_distance - ent.moveinfo.current_speed) >= ent.moveinfo.decel_distance)
			return;

		// during this move we will accelrate from current_speed to move_speed
		// and cross over the decel_distance; figure the average speed for the
		// entire move
		float p1_distance = ent.moveinfo.remaining_distance - ent.moveinfo.decel_distance;
		float p1_speed = (old_speed + ent.moveinfo.move_speed) / 2.0f;
		float p2_distance = ent.moveinfo.move_speed * (1.0f - (p1_distance / p1_speed));
		float distance = p1_distance + p2_distance;
		ent.moveinfo.current_speed = (p1_speed * (p1_distance / distance)) + (ent.moveinfo.move_speed * (p2_distance / distance));
		ent.moveinfo.next_speed = ent.moveinfo.move_speed - ent.moveinfo.decel * (p2_distance / distance);
		return;
	}

	// we are at constant velocity (move_speed)
}

static void Think_AccelMove(entity &ent)
{
	ent.moveinfo.remaining_distance -= ent.moveinfo.current_speed;

	plat_Accelerate(ent);

	// will the entire move complete on next frame?
	if (ent.moveinfo.remaining_distance <= ent.moveinfo.current_speed)
	{
		Move_Final(ent);
		return;
	}

	ent.velocity = ent.moveinfo.dir * (ent.moveinfo.current_speed * 10);
	ent.nextthink = level.framenum + 1_hz;
	ent.think = SAVABLE(Think_AccelMove);
}

static void plat_go_down(entity &ent);

REGISTER_STATIC_SAVABLE(plat_go_down);

static void plat_hit_top(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_end)
			gi.sound(ent, CHAN_VOICE | CHAN_NO_PHS_ADD, ent.moveinfo.sound_end, ATTN_STATIC);
		ent.sound = SOUND_NONE;
	}
	ent.moveinfo.state = STATE_TOP;

	ent.think = SAVABLE(plat_go_down);
	ent.nextthink = level.framenum + 3s;
}

REGISTER_STATIC_SAVABLE(plat_hit_top);

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
// from rogue/func.qc
//void plat2_kill_danger_area(entity &ent);
//void plat2_spawn_danger_area(entity &ent);
#endif

static void plat_hit_bottom(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_end)
			gi.sound(ent, CHAN_VOICE | CHAN_NO_PHS_ADD, ent.moveinfo.sound_end, ATTN_STATIC);
		ent.sound = SOUND_NONE;
	}
	ent.moveinfo.state = STATE_BOTTOM;
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)

	//plat2_kill_danger_area (ent);
#endif
}

REGISTER_STATIC_SAVABLE(plat_hit_bottom);

static void plat_go_down(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_start)
			gi.sound(ent, CHAN_VOICE | CHAN_NO_PHS_ADD, ent.moveinfo.sound_start, ATTN_STATIC);
		ent.sound = ent.moveinfo.sound_middle;
	}
	ent.moveinfo.state = STATE_DOWN;
	Move_Calc(ent, ent.moveinfo.end_origin, SAVABLE(plat_hit_bottom));
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)

	//plat2_spawn_danger_area(ent);
#endif
}

static void plat_go_up(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_start)
			gi.sound(ent, CHAN_VOICE | CHAN_NO_PHS_ADD, ent.moveinfo.sound_start, ATTN_STATIC);
		ent.sound = ent.moveinfo.sound_middle;
	}
	ent.moveinfo.state = STATE_UP;
	Move_Calc(ent, ent.moveinfo.start_origin, SAVABLE(plat_hit_top));
}

static void plat_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

#ifdef GROUND_ZERO
	// gib dead things
	if(other.health < 1)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100, 1, { DAMAGE_NONE }, MOD_CRUSH);
#endif

	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);

	if (self.moveinfo.state == STATE_UP)
		plat_go_down(self);
	else if (self.moveinfo.state == STATE_DOWN)
		plat_go_up(self);
}

REGISTER_STATIC_SAVABLE(plat_blocked);

void Use_Plat(entity &ent, entity &other [[maybe_unused]], entity &)
{
#ifdef ROGUE_AI
	// if a monster is using us, then allow the activity when stopped.
	if (other.svflags & SVF_MONSTER)
	{
		if (ent.moveinfo.state == STATE_TOP)
			plat_go_down (ent);
		else if (ent.moveinfo.state == STATE_BOTTOM)
			plat_go_up (ent);

		return;
	}
#endif
	
	if (ent.think)
		return;     // already down
	plat_go_down(ent);
}

REGISTER_STATIC_SAVABLE(Use_Plat);

static void Touch_Plat_Center(entity &ent, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (other.health <= 0)
		return;

	entity &plat = ent.enemy;   // now point at the plat, not the trigger

	if (plat.moveinfo.state == STATE_BOTTOM)
		plat_go_up(plat);
	else if (plat.moveinfo.state == STATE_TOP)
		plat.nextthink = level.framenum + 1s;   // the player is still on the plat, so delay going down
}

REGISTER_STATIC_SAVABLE(Touch_Plat_Center);

entity &plat_spawn_inside_trigger(entity &ent)
{
//
// middle trigger
//
	entity &trigger = G_Spawn();
	trigger.touch = SAVABLE(Touch_Plat_Center);
	trigger.movetype = MOVETYPE_NONE;
	trigger.solid = SOLID_TRIGGER;
	trigger.enemy = ent;

	vector tmin, tmax;

	tmin.x = ent.bounds.mins.x + 25;
	tmin.y = ent.bounds.mins.y + 25;
	tmin.z = ent.bounds.mins.z;

	tmax.x = ent.bounds.maxs.x - 25;
	tmax.y = ent.bounds.maxs.y - 25;
	tmax.z = ent.bounds.maxs.z + 8;

	tmin.z = tmax.z - (ent.pos1.z - ent.pos2.z + st.lip);

	if (ent.spawnflags & PLAT_LOW_TRIGGER)
		tmax.z = tmin.z + 8;

	if (tmax.x - tmin.x <= 0)
	{
		tmin.x = (ent.bounds.mins.x + ent.bounds.maxs.x) * 0.5f;
		tmax.x = tmin.x + 1;
	}
	if (tmax.y - tmin.y <= 0)
	{
		tmin.y = (ent.bounds.mins.y + ent.bounds.maxs.y) * 0.5f;
		tmax.y = tmin.y + 1;
	}

	trigger.bounds = { tmin, tmax };

	gi.linkentity(trigger);
	return trigger;
}

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed   default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

"speed" overrides default 200.
"accel" overrides default 500
"lip"   overrides default 8 pixel lip

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determoveinfoned by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/
static void SP_func_plat(entity &ent)
{
	ent.angles = vec3_origin;
	ent.solid = SOLID_BSP;
	ent.movetype = MOVETYPE_PUSH;

	gi.setmodel(ent, ent.model);

	ent.blocked = SAVABLE(plat_blocked);

	if (!ent.speed)
		ent.speed = 20.f;
	else
		ent.speed *= 0.1f;

	if (!ent.accel)
		ent.accel = 5.f;
	else
		ent.accel *= 0.1f;

	if (!ent.decel)
		ent.decel = 5.f;
	else
		ent.decel *= 0.1f;

	if (!ent.dmg)
		ent.dmg = 2;

	if (!st.lip)
		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	ent.pos1 = ent.origin;
	ent.pos2 = ent.origin;
	if (st.height)
		ent.pos2.z -= st.height;
	else
		ent.pos2.z -= (ent.bounds.maxs.z - ent.bounds.mins.z) - st.lip;

	ent.use = SAVABLE(Use_Plat);

	plat_spawn_inside_trigger(ent);     // the "start moving" trigger

	if (ent.targetname)
		ent.moveinfo.state = STATE_UP;
	else
	{
		ent.origin = ent.pos2;
		gi.linkentity(ent);
		ent.moveinfo.state = STATE_BOTTOM;
	}

	ent.moveinfo.speed = ent.speed;
	ent.moveinfo.accel = ent.accel;
	ent.moveinfo.decel = ent.decel;
	ent.moveinfo.wait = ent.wait;
	ent.moveinfo.start_origin = ent.pos1;
	ent.moveinfo.start_angles = ent.angles;
	ent.moveinfo.end_origin = ent.pos2;
	ent.moveinfo.end_angles = ent.angles;

	ent.moveinfo.sound_start = gi.soundindex("plats/pt1_strt.wav");
	ent.moveinfo.sound_middle = gi.soundindex("plats/pt1_mid.wav");
	ent.moveinfo.sound_end = gi.soundindex("plats/pt1_end.wav");
}

REGISTER_ENTITY(FUNC_PLAT, func_plat);

//====================================================================

/*QUAKED func_rotating (0 .5 .8) ? START_ON REVERSE X_AXIS Y_AXIS TOUCH_PAIN STOP ANIMATED ANIMATED_FAST
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"speed" determines how fast it moves; default value is 100.
"dmg"   damage to inflict when blocked (2 default)

REVERSE will cause the it to rotate in the opposite direction.
STOP mean it will stop moving instead of pushing entities
*/

constexpr spawn_flag ROTATING_START_ON = (spawn_flag)1;
constexpr spawn_flag ROTATING_REVERSE = (spawn_flag)2;
constexpr spawn_flag ROTATING_X_AXIS = (spawn_flag)4;
constexpr spawn_flag ROTATING_Y_AXIS = (spawn_flag)8;
constexpr spawn_flag ROTATING_TOUCH_PAIN = (spawn_flag)16;
constexpr spawn_flag ROTATING_STOP = (spawn_flag)32;
constexpr spawn_flag ROTATING_ANIMATED = (spawn_flag)64;
constexpr spawn_flag ROTATING_ANIMATED_FAST = (spawn_flag)128;

#ifdef GROUND_ZERO
static void rotating_accel(entity &self);

REGISTER_STATIC_SAVABLE(rotating_accel);

static void rotating_accel(entity &self)
{
	float current_speed = VectorLength (self.avelocity);

	if (current_speed >= (self.speed - self.accel))		// done
	{
		self.avelocity = self.movedir * self.speed;
		G_UseTargets (self, self);
	}
	else
	{
		current_speed += self.accel;
		self.avelocity = self.movedir * current_speed;
		self.think = SAVABLE(rotating_accel);
		self.nextthink = level.framenum + 1_hz;
	}
}
static void rotating_decel(entity &self);

REGISTER_STATIC_SAVABLE(rotating_decel);

static void rotating_decel(entity &self)
{
	float current_speed = VectorLength (self.avelocity);

	// done
	if (current_speed <= self.decel)
	{
		self.avelocity = vec3_origin;
		G_UseTargets (self, self);
		self.touch = nullptr;
	}
	else
	{
		current_speed -= self.decel;
		self.avelocity = self.movedir * current_speed;
		self.think = SAVABLE(rotating_decel);
		self.nextthink = level.framenum + 1_hz;
	}
}
#endif

static void rotating_blocked(entity &self, entity &other)
{
	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, (int32_t)self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(rotating_blocked);

static void rotating_touch(entity &self, entity &other, vector, const surface &)
{
	if (self.avelocity)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, (int32_t)self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(rotating_touch);

static void rotating_use(entity &self, entity &, entity &)
{
	if (self.avelocity)
	{
		self.sound = SOUND_NONE;
#ifdef GROUND_ZERO
		if(self.spawnflags & DOOR_INACTIVE)	// Decelerate
			rotating_decel (self);
		else
		{
			G_UseTargets (self, self);
#endif
			self.avelocity = vec3_origin;
			self.touch = nullptr;
#ifdef GROUND_ZERO
		}
#endif
	}
	else
	{
		self.sound = self.moveinfo.sound_middle;

#ifdef GROUND_ZERO
		if(self.spawnflags & DOOR_INACTIVE)	// accelerate
			rotating_accel (self);
		else
		{
			G_UseTargets (self, self);
#endif
			self.avelocity = self.movedir * self.speed;
#ifdef GROUND_ZERO
		}
#endif
		if (self.spawnflags & ROTATING_TOUCH_PAIN)
			self.touch = SAVABLE(rotating_touch);
	}
}

REGISTER_STATIC_SAVABLE(rotating_use);

static void SP_func_rotating(entity &ent)
{
	ent.solid = SOLID_BSP;
	if (ent.spawnflags & ROTATING_STOP)
		ent.movetype = MOVETYPE_STOP;
	else
		ent.movetype = MOVETYPE_PUSH;

	// set the axis of rotation
	ent.movedir = vec3_origin;
	if (ent.spawnflags & ROTATING_X_AXIS)
		ent.movedir.z = 1.0f;
	else if (ent.spawnflags & ROTATING_Y_AXIS)
		ent.movedir.x = 1.0f;
	else // Z_AXIS
		ent.movedir.y = 1.0f;

	// check for reverse rotation
	if (ent.spawnflags & ROTATING_REVERSE)
		ent.movedir = -ent.movedir;

	if (!ent.speed)
		ent.speed = 100.f;
	if (!ent.dmg)
		ent.dmg = 2;

	ent.use = SAVABLE(rotating_use);
	if (ent.dmg)
		ent.blocked = SAVABLE(rotating_blocked);

	if (ent.spawnflags & ROTATING_START_ON)
		ent.use(ent, world, world);

	if (ent.spawnflags & ROTATING_ANIMATED)
		ent.effects |= EF_ANIM_ALL;
	if (ent.spawnflags & ROTATING_ANIMATED_FAST)
		ent.effects |= EF_ANIM_ALLFAST;

#ifdef GROUND_ZERO
	if(ent.spawnflags & DOOR_INACTIVE)	// Accelerate / Decelerate
	{
		if(!ent.accel)
			ent.accel = 1.f;
		else if (ent.accel > ent.speed)
			ent.accel = ent.speed;

		if(!ent.decel)
			ent.decel = 1.f;
		else if (ent.decel > ent.speed)
			ent.decel = ent.speed;
	}
#endif

	gi.setmodel(ent, ent.model);
	gi.linkentity(ent);
}

static REGISTER_ENTITY(FUNC_ROTATING, func_rotating);

/*
======================================================================

BUTTONS

======================================================================
*/

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"     determines the opening direction
"target"    all entities with a matching targetname will be used
"speed"     override the default 40 speed
"wait"      override the default 1 second wait (-1 = never return)
"lip"       override the default 4 pixel lip remaining at end of move
"health"    if set, the button must be killed instead of touched
"sounds"
1) silent
2) steam metal
3) wooden clunk
4) metallic click
5) in-out
*/

static void button_done(entity &self)
{
	self.moveinfo.state = STATE_BOTTOM;
	self.effects &= ~EF_ANIM23;
	self.effects |= EF_ANIM01;
}

REGISTER_STATIC_SAVABLE(button_done);

static void button_return(entity &self)
{
	self.moveinfo.state = STATE_DOWN;

	Move_Calc(self, self.moveinfo.start_origin, SAVABLE(button_done));

	self.frame = 0;

	if (self.health)
		self.takedamage = true;
}

REGISTER_STATIC_SAVABLE(button_return);

static void button_wait(entity &self)
{
	self.moveinfo.state = STATE_TOP;
	self.effects &= ~EF_ANIM01;
	self.effects |= EF_ANIM23;

	G_UseTargets(self, self.activator);
	self.frame = 1;
	if (self.moveinfo.wait >= gtimef::zero())
	{
		self.nextthink = duration_cast<gtime>(level.framenum + self.moveinfo.wait);
		self.think = SAVABLE(button_return);
	}
}

REGISTER_STATIC_SAVABLE(button_wait);

static void button_fire(entity &self)
{
	if (self.moveinfo.state == STATE_UP || self.moveinfo.state == STATE_TOP)
		return;

	self.moveinfo.state = STATE_UP;
	if (self.moveinfo.sound_start && !(self.flags & FL_TEAMSLAVE))
		gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_start, ATTN_STATIC);
	Move_Calc(self, self.moveinfo.end_origin, SAVABLE(button_wait));
}

static void button_use(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;
	button_fire(self);
}

REGISTER_STATIC_SAVABLE(button_use);

static void button_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (other.health <= 0)
		return;

	self.activator = other;
	button_fire(self);
}

REGISTER_STATIC_SAVABLE(button_touch);

static void button_killed(entity &self, entity &, entity &attacker, int32_t, vector)
{
	self.activator = attacker;
	self.health = self.max_health;
	self.takedamage = false;
	button_fire(self);
}

REGISTER_STATIC_SAVABLE(button_killed);

static void SP_func_button(entity &ent)
{
	G_SetMovedir(ent.angles, ent.movedir);
	ent.movetype = MOVETYPE_STOP;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.model);

	if (ent.sounds != 1)
		ent.moveinfo.sound_start = gi.soundindex("switches/butn2.wav");

	if (!ent.speed)
		ent.speed = 40.f;
	if (!ent.accel)
		ent.accel = ent.speed;
	if (!ent.decel)
		ent.decel = ent.speed;

	if (ent.wait == gtimef::zero())
		ent.wait = 3s;
	if (!st.lip)
		st.lip = 4;

	ent.pos1 = ent.origin;
	vector	abs_movedir;
	abs_movedir.x = fabs(ent.movedir.x);
	abs_movedir.y = fabs(ent.movedir.y);
	abs_movedir.z = fabs(ent.movedir.z);
	float dist = abs_movedir.x * ent.size.x + abs_movedir.y * ent.size.y + abs_movedir.z * ent.size.z - st.lip;
	ent.pos2 = ent.pos1 + (dist * ent.movedir);

	ent.use = SAVABLE(button_use);
	ent.effects |= EF_ANIM01;

	if (ent.health)
	{
		ent.max_health = ent.health;
		ent.die = SAVABLE(button_killed);
		ent.takedamage = true;
	}
	else if (!ent.targetname)
		ent.touch = SAVABLE(button_touch);

	ent.moveinfo.state = STATE_BOTTOM;

	ent.moveinfo.speed = ent.speed;
	ent.moveinfo.accel = ent.accel;
	ent.moveinfo.decel = ent.decel;
	ent.moveinfo.wait = ent.wait;
	ent.moveinfo.start_origin = ent.pos1;
	ent.moveinfo.start_angles = ent.angles;
	ent.moveinfo.end_origin = ent.pos2;
	ent.moveinfo.end_angles = ent.angles;

	gi.linkentity(ent);
}

static REGISTER_ENTITY(FUNC_BUTTON, func_button);

/*
======================================================================

DOORS

  spawn a trigger surrounding the entire team unless it is
  already targeted by another

======================================================================
*/

/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER NOMONSTER ANIMATED TOGGLE ANIMATED_FAST
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"message"   is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"     determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"    if set, door must be shot open
"speed"     movement speed (100 default)
"wait"      wait before returning (3 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"sounds"
1)  silent
2)  light
3)  medium
4)  heavy
*/

constexpr spawn_flag DOOR_ANIMATED = (spawn_flag)16;
constexpr spawn_flag DOOR_ANIMATED_FAST = (spawn_flag)64;

static void door_use_areaportals(entity &self, bool open)
{
	if (!self.target)
		return;
	
	entityref t;
	while ((t = G_FindFunc<&entity::targetname>(t, self.target, striequals)).has_value())
		if (t->type == ET_FUNC_AREAPORTAL)
			gi.SetAreaPortalState(t->style, open);
}

static void door_go_down(entity &self);

REGISTER_STATIC_SAVABLE(door_go_down);

static void door_hit_top(entity &self)
{
	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_end)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_end, ATTN_STATIC);
		self.sound = SOUND_NONE;
	}
	
	self.moveinfo.state = STATE_TOP;
	
	if (self.spawnflags & DOOR_TOGGLE)
		return;

	if (self.moveinfo.wait >= gtimef::zero())
	{
		self.think = SAVABLE(door_go_down);
		self.nextthink = duration_cast<gtime>(level.framenum + self.moveinfo.wait);
	}
}

REGISTER_STATIC_SAVABLE(door_hit_top);

static void door_hit_bottom(entity &self)
{
	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_end)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_end, ATTN_STATIC);
		self.sound = SOUND_NONE;
	}
	self.moveinfo.state = STATE_BOTTOM;
	door_use_areaportals(self, false);
}

REGISTER_STATIC_SAVABLE(door_hit_bottom);

static void door_go_down(entity &self)
{
	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_start)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_start, ATTN_STATIC);
		self.sound = self.moveinfo.sound_middle;
	}
	if (self.max_health)
	{
		self.takedamage = true;
		self.health = self.max_health;
	}

	self.moveinfo.state = STATE_DOWN;

	if (self.type == ET_FUNC_DOOR)
		Move_Calc(self, self.moveinfo.start_origin, SAVABLE(door_hit_bottom));
	else if (self.type == ET_FUNC_DOOR_ROTATING)
		AngleMove_Calc(self, SAVABLE(door_hit_bottom));
}

static void door_go_up(entity &self, entity &cactivator)
{
	if (self.moveinfo.state == STATE_UP)
		return;     // already going up

	if (self.moveinfo.state == STATE_TOP)
	{
		// reset top wait time
		if (self.moveinfo.wait >= gtimef::zero())
			self.nextthink = duration_cast<gtime>(level.framenum + self.moveinfo.wait);
		return;
	}

	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_start)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_start, ATTN_STATIC);
		self.sound = self.moveinfo.sound_middle;
	}

	self.moveinfo.state = STATE_UP;

	if (self.type == ET_FUNC_DOOR)
		Move_Calc(self, self.moveinfo.end_origin, SAVABLE(door_hit_top));
	else if (self.type == ET_FUNC_DOOR_ROTATING)
		AngleMove_Calc(self, SAVABLE(door_hit_top));

	G_UseTargets(self, cactivator);
	door_use_areaportals(self, true);
}

#ifdef GROUND_ZERO
static void smart_water_go_up(entity &self);

REGISTER_STATIC_SAVABLE(smart_water_go_up);

static void smart_water_go_up(entity &self)
{
	float	distance;
	entityref	lowestPlayer;
	float	lowestPlayerPt;

	if (self.moveinfo.state == STATE_TOP)
	{	// reset top wait time
		if (self.moveinfo.wait >= gtimef::zero())
			self.nextthink = duration_cast<gtime>(level.framenum + self.moveinfo.wait);
		return;
	}

	if (self.health)
	{
		if(self.absbounds.maxs[2] >= self.health)
		{
			self.velocity = vec3_origin;
			self.nextthink = gtime::zero();
			self.moveinfo.state = STATE_TOP;
			return;
		}
	}

	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_start)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_start, ATTN_STATIC);
		self.sound = self.moveinfo.sound_middle;
	}

	// find the lowest player point.
	lowestPlayerPt = FLT_MAX;

	for (entity &ent : entity_range(1, game.maxclients))
	{
		// don't count dead or unused player slots
		if (ent.inuse && ent.health > 0)
		{
			if (ent.absbounds.mins[2] < lowestPlayerPt)
			{
				lowestPlayerPt = ent.absbounds.mins[2];
				lowestPlayer = ent;
			}
		}
	}

	if (!lowestPlayer.has_value())
		return;

	distance = lowestPlayerPt - self.absbounds.maxs[2];

	// for the calculations, make sure we intend to go up at least a little.
	if(distance < self.accel)
	{
		distance = 100.f;
		self.moveinfo.speed = 5.f;
	}
	else
		self.moveinfo.speed = distance / self.accel;

	if(self.moveinfo.speed < 5)
		self.moveinfo.speed = 5.f;
	else if(self.moveinfo.speed > self.speed)
		self.moveinfo.speed = self.speed;

	// FIXME - should this allow any movement other than straight up?
	self.moveinfo.dir = MOVEDIR_UP;	
	self.velocity = self.moveinfo.dir * self.moveinfo.speed;
	self.moveinfo.remaining_distance = distance;

	if(self.moveinfo.state != STATE_UP)
	{
		G_UseTargets (self, lowestPlayer);
		door_use_areaportals (self, true);
		self.moveinfo.state = STATE_UP;
	}

	self.think = SAVABLE(smart_water_go_up);
	self.nextthink = level.framenum + 1_hz;
}
#endif

static void door_use(entity &self, entity &, entity &cactivator)
{
	if (self.flags & FL_TEAMSLAVE)
		return;

	if (self.spawnflags & DOOR_TOGGLE)
	{
		if (self.moveinfo.state == STATE_UP || self.moveinfo.state == STATE_TOP)
		{
			// trigger all paired doors
			for (entityref ent = self; ent.has_value(); ent = ent->teamchain)
			{
				ent->message = nullptr;
				ent->touch = nullptr;
				door_go_down(ent);
			}
			return;
		}
	}

#ifdef GROUND_ZERO
	// smart water is different
	if ((self.spawnflags & WATER_SMART) && (gi.pointcontents(self.bounds.center()) & MASK_WATER))
	{
		self.message = 0;
		self.touch = nullptr;
		self.enemy = cactivator;
		smart_water_go_up (self);
		return;
	}
#endif

	// trigger all paired doors
	for (entityref ent = self; ent.has_value(); ent = ent->teamchain)
	{
		ent->message = nullptr;
		ent->touch = nullptr;
		door_go_up(ent, cactivator);
	}
}

REGISTER_STATIC_SAVABLE(door_use);

static void Touch_DoorTrigger(entity &self, entity &other, vector, const surface &)
{
	if (other.health <= 0)
		return;

	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
		return;

	if ((self.owner->spawnflags & DOOR_NOMONSTER) && (other.svflags & SVF_MONSTER))
		return;

	if (level.framenum < self.touch_debounce_framenum)
		return;

	self.touch_debounce_framenum = level.framenum + 1s;
	door_use(self.owner, other, other);
}

REGISTER_STATIC_SAVABLE(Touch_DoorTrigger);

static void Think_CalcMoveSpeed(entity &self)
{
	if (self.flags & FL_TEAMSLAVE)
		return;     // only the team master does this

	// find the smallest distance any member of the team will be moving
	float min = fabs(self.moveinfo.distance);
	for (entityref ent = self.teamchain; ent.has_value(); ent = ent->teamchain)
	{
		float dist = fabs(ent->moveinfo.distance);
		if (dist < min)
			min = dist;
	}

	float time = min / self.moveinfo.speed;

	// adjust speeds so they will all complete at the same time
	for (entityref ent = self; ent.has_value(); ent = ent->teamchain)
	{
		float newspeed = fabs(ent->moveinfo.distance) / time;
		float ratio = newspeed / ent->moveinfo.speed;
		if (ent->moveinfo.accel == ent->moveinfo.speed)
			ent->moveinfo.accel = newspeed;
		else
			ent->moveinfo.accel *= ratio;
		if (ent->moveinfo.decel == ent->moveinfo.speed)
			ent->moveinfo.decel = newspeed;
		else
			ent->moveinfo.decel *= ratio;
		ent->moveinfo.speed = newspeed;
	}
}

REGISTER_STATIC_SAVABLE(Think_CalcMoveSpeed);

static void Think_SpawnDoorTrigger(entity &ent)
{
	if (ent.flags & FL_TEAMSLAVE)
		return;     // only the team leader spawns a trigger

	bbox cbounds = ent.absbounds;

	for (entityref other = ent.teamchain; other.has_value(); other = other->teamchain)
		cbounds += other->absbounds;

	// expand
	cbounds.mins.x -= 60;
	cbounds.mins.y -= 60;
	cbounds.maxs.x += 60;
	cbounds.maxs.y += 60;

	entity &other = G_Spawn();
	other.bounds = cbounds;
	other.owner = ent;
	other.solid = SOLID_TRIGGER;
	other.movetype = MOVETYPE_NONE;
	other.touch = SAVABLE(Touch_DoorTrigger);
	gi.linkentity(other);

	if (ent.spawnflags & DOOR_START_OPEN)
		door_use_areaportals(ent, true);

	Think_CalcMoveSpeed(ent);
}

REGISTER_STATIC_SAVABLE(Think_SpawnDoorTrigger);

static void door_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);

	if (self.spawnflags & DOOR_CRUSHER)
		return;

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self.moveinfo.wait >= gtimef::zero())
	{
		if (self.moveinfo.state == STATE_DOWN)
			for (entityref ent = self.teammaster; ent.has_value(); ent = ent->teamchain)
				door_go_up(ent, ent->activator.or_default(world));
		else
			for (entityref ent = self.teammaster ; ent.has_value(); ent = ent->teamchain)
				door_go_down(ent);
	}
}

REGISTER_STATIC_SAVABLE(door_blocked);

static void door_killed(entity &self, entity &, entity &attacker, int32_t, vector)
{
	for (entityref ent = self.teammaster; ent.has_value(); ent = ent->teamchain)
	{
		ent->health = ent->max_health;
		ent->takedamage = false;
	}

	door_use(self.teammaster, attacker, attacker);
}

REGISTER_STATIC_SAVABLE(door_killed);

static void door_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (level.framenum < self.touch_debounce_framenum)
		return;
	self.touch_debounce_framenum = level.framenum + 5s;

	gi.centerprint(other, self.message);
	gi.sound(other, gi.soundindex("misc/talk1.wav"));
}

REGISTER_STATIC_SAVABLE(door_touch);

static void SP_func_door(entity &ent)
{
	if (ent.sounds != 1)
	{
		ent.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		ent.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		ent.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	G_SetMovedir(ent.angles, ent.movedir);
	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.model);

	ent.blocked = SAVABLE(door_blocked);
	ent.use = SAVABLE(door_use);

	if (!ent.speed)
		ent.speed = 100.f;
#ifdef SINGLE_PLAYER
	if (deathmatch)
#endif
		ent.speed *= 2;

	if (!ent.accel)
		ent.accel = ent.speed;
	if (!ent.decel)
		ent.decel = ent.speed;

	if (ent.wait == gtimef::zero())
		ent.wait = 3s;
	if (!st.lip)
		st.lip = 8;
	if (!ent.dmg)
		ent.dmg = 2;

	// calculate second position
	ent.pos1 = ent.origin;
	vector abs_movedir;
	abs_movedir.x = fabs(ent.movedir.x);
	abs_movedir.y = fabs(ent.movedir.y);
	abs_movedir.z = fabs(ent.movedir.z);
	ent.moveinfo.distance = abs_movedir.x * ent.size.x + abs_movedir.y * ent.size.y + abs_movedir.z * ent.size.z - st.lip;
	ent.pos2 = ent.pos1 + (ent.moveinfo.distance * ent.movedir);

	// if it starts open, switch the positions
	if (ent.spawnflags & DOOR_START_OPEN)
	{
		ent.origin = ent.pos2;
		ent.pos2 = ent.pos1;
		ent.pos1 = ent.origin;
	}

	ent.moveinfo.state = STATE_BOTTOM;

	if (ent.health)
	{
		ent.takedamage = true;
		ent.die = SAVABLE(door_killed);
		ent.max_health = ent.health;
	}
	else if (ent.targetname && ent.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.touch = SAVABLE(door_touch);
	}

	ent.moveinfo.speed = ent.speed;
	ent.moveinfo.accel = ent.accel;
	ent.moveinfo.decel = ent.decel;
	ent.moveinfo.wait = ent.wait;
	ent.moveinfo.start_origin = ent.pos1;
	ent.moveinfo.start_angles = ent.angles;
	ent.moveinfo.end_origin = ent.pos2;
	ent.moveinfo.end_angles = ent.angles;

	if (ent.spawnflags & DOOR_ANIMATED)
		ent.effects |= EF_ANIM_ALL;
	if (ent.spawnflags & DOOR_ANIMATED_FAST)
		ent.effects |= EF_ANIM_ALLFAST;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent.team)
		ent.teammaster = ent;

	gi.linkentity(ent);

	ent.nextthink = level.framenum + 1_hz;
	if (ent.health || ent.targetname)
		ent.think = SAVABLE(Think_CalcMoveSpeed);
	else
		ent.think = SAVABLE(Think_SpawnDoorTrigger);
}

REGISTER_ENTITY(FUNC_DOOR, func_door);

/*QUAKED func_door_rotating (0 .5 .8) ? START_OPEN REVERSE CRUSHER NOMONSTER ANIMATED TOGGLE X_AXIS Y_AXIS
TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"message"   is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"     determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"    if set, door must be shot open
"speed"     movement speed (100 default)
"wait"      wait before returning (3 default, -1 = never return)
"dmg"       damage to inflict when blocked (2 default)
"sounds"
1)  silent
2)  light
3)  medium
4)  heavy
*/

constexpr spawn_flag DOOR_ROTATING_ANIMATED = (spawn_flag)16;

#ifdef GROUND_ZERO
static void Door_Activate(entity &self, entity &, entity &)
{
	self.use = nullptr;
	
	if (self.health)
	{
		self.takedamage = true;
		self.die = SAVABLE(door_killed);
		self.max_health = self.health;
	}

	if (self.health)
		self.think = SAVABLE(Think_CalcMoveSpeed);
	else
		self.think = SAVABLE(Think_SpawnDoorTrigger);
	self.nextthink = level.framenum + 1_hz;

}

REGISTER_STATIC_SAVABLE(Door_Activate);
#endif

static void SP_func_door_rotating(entity &ent)
{
	ent.angles = vec3_origin;

	// set the axis of rotation
	ent.movedir = vec3_origin;
	if (ent.spawnflags & DOOR_X_AXIS)
		ent.movedir.z = 1.0f;
	else if (ent.spawnflags & DOOR_Y_AXIS)
		ent.movedir.x = 1.0f;
	else // Z_AXIS
		ent.movedir.y = 1.0f;

	// check for reverse rotation
	if (ent.spawnflags & DOOR_REVERSE)
		ent.movedir = -ent.movedir;

	if (!st.distance)
	{
		gi.dprintfmt("{}: no distance set\n", ent);
		st.distance = 90;
	}

	ent.pos1 = ent.angles;
	ent.pos2 = ent.angles + (st.distance * ent.movedir);
	ent.moveinfo.distance = (float)st.distance;

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.model);

	ent.blocked = SAVABLE(door_blocked);
	ent.use = SAVABLE(door_use);

	if (!ent.speed)
		ent.speed = 100.f;
	if (!ent.accel)
		ent.accel = ent.speed;
	if (!ent.decel)
		ent.decel = ent.speed;

	if (ent.wait == gtimef::zero())
		ent.wait = 3s;
	if (!ent.dmg)
		ent.dmg = 2;

	if (ent.sounds != 1)
	{
		ent.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		ent.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		ent.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	// if it starts open, switch the positions
	if (ent.spawnflags & DOOR_START_OPEN)
	{
		ent.angles = ent.pos2;
		ent.pos2 = ent.pos1;
		ent.pos1 = ent.angles;
		ent.movedir = -ent.movedir;
	}

	if (ent.health)
	{
		ent.takedamage = true;
		ent.die = SAVABLE(door_killed);
		ent.max_health = ent.health;
	}

	if (ent.targetname && ent.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.touch = SAVABLE(door_touch);
	}

	ent.moveinfo.state = STATE_BOTTOM;
	ent.moveinfo.speed = ent.speed;
	ent.moveinfo.accel = ent.accel;
	ent.moveinfo.decel = ent.decel;
	ent.moveinfo.wait = ent.wait;
	ent.moveinfo.start_origin = ent.origin;
	ent.moveinfo.start_angles = ent.pos1;
	ent.moveinfo.end_origin = ent.origin;
	ent.moveinfo.end_angles = ent.pos2;

	if (ent.spawnflags & DOOR_ROTATING_ANIMATED)
		ent.effects |= EF_ANIM_ALL;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent.team)
		ent.teammaster = ent;

	gi.linkentity(ent);

	ent.nextthink = level.framenum + 1_hz;
	if (ent.health || ent.targetname)
		ent.think = SAVABLE(Think_CalcMoveSpeed);
	else
		ent.think = SAVABLE(Think_SpawnDoorTrigger);

#ifdef GROUND_ZERO
	if (ent.spawnflags & DOOR_INACTIVE)
	{
		ent.takedamage = false;
		ent.die = nullptr;
		ent.think = nullptr;
		ent.nextthink = gtime::zero();
		ent.use = SAVABLE(Door_Activate);
	}
#endif
}

REGISTER_ENTITY(FUNC_DOOR_ROTATING, func_door_rotating);

/*QUAKED func_water (0 .5 .8) ? START_OPEN
func_water is a moveable water brush.  It must be targeted to operate.  Use a non-water texture at your own risk.

START_OPEN causes the water to move to its destination when spawned and operate in reverse.

"angle"     determines the opening direction (up or down only)
"speed"     movement speed (25 default)
"wait"      wait before returning (-1 default, -1 = TOGGLE)
"lip"       lip remaining at end of move (0 default)
"sounds"    (yes, these need to be changed)
0)  no sound
1)  water
2)  lava
*/

#ifdef GROUND_ZERO
static void smart_water_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && (!other.is_client()) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_LAVA);
		// if it's still there, nuke it
		if (other.inuse)		// PGM
			BecomeExplosion1 (other);
		return;
	}

	T_Damage (other, self, self, vec3_origin, other.origin, vec3_origin, 100, 1, { DAMAGE_NONE }, MOD_LAVA);
}

REGISTER_STATIC_SAVABLE(smart_water_blocked);
#endif

static void SP_func_water(entity &self)
{
	G_SetMovedir(self.angles, self.movedir);
	self.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	gi.setmodel(self, self.model);

	switch (self.sounds)
	{
	case 1: // water
		self.moveinfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self.moveinfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;

	case 2: // lava
		self.moveinfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self.moveinfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;
	}

	// calculate second position
	self.pos1 = self.origin;
	vector	abs_movedir;
	abs_movedir.x = fabs(self.movedir.x);
	abs_movedir.y = fabs(self.movedir.y);
	abs_movedir.z = fabs(self.movedir.z);
	self.moveinfo.distance = abs_movedir.x * self.size.x + abs_movedir.y * self.size.y + abs_movedir.z * self.size.z - st.lip;
	self.pos2 = self.pos1 + (self.moveinfo.distance * self.movedir);

	// if it starts open, switch the positions
	if (self.spawnflags & DOOR_START_OPEN)
	{
		self.origin = self.pos2;
		self.pos2 = self.pos1;
		self.pos1 = self.origin;
	}

	self.moveinfo.start_origin = self.pos1;
	self.moveinfo.start_angles = self.angles;
	self.moveinfo.end_origin = self.pos2;
	self.moveinfo.end_angles = self.angles;

	self.moveinfo.state = STATE_BOTTOM;

	if (!self.speed)
		self.speed = 25.f;
	self.moveinfo.accel = self.moveinfo.decel = self.moveinfo.speed = self.speed;

#ifdef GROUND_ZERO
	if (self.spawnflags & WATER_SMART)	// smart water
	{
		// this is actually the divisor of the lowest player's distance to determine speed.
		// self.speed then becomes the cap of the speed.
		if(!self.accel)
			self.accel = 20.f;
		self.blocked = SAVABLE(smart_water_blocked);
	}
#endif

	if (self.wait == gtimef::zero())
		self.wait = -1s;
	self.moveinfo.wait = self.wait;

	self.use = SAVABLE(door_use);

	if (self.wait == -1s)
		self.spawnflags |= DOOR_TOGGLE;

	gi.linkentity(self);
}

static REGISTER_ENTITY_INHERIT(FUNC_WATER, func_water, ET_FUNC_DOOR);

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed   default 100
dmg     default 2
noise   looping sound to play when the train is in motion

*/

static void train_next(entity &self);

REGISTER_STATIC_SAVABLE(train_next);

static void train_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	if (level.framenum < self.touch_debounce_framenum)
		return;

	if (!self.dmg)
		return;
	self.touch_debounce_framenum = level.framenum + 500ms;
	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(train_blocked);

static void train_wait(entity &self)
{
	if (self.target_ent->pathtarget)
	{
		entity &ent = self.target_ent;
		string savetarget = ent.target;
		ent.target = ent.pathtarget;
		G_UseTargets(ent, self.activator);
		ent.target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self.inuse)
			return;
	}

	if (self.moveinfo.wait != gtimef::zero())
	{
		if (self.moveinfo.wait > gtimef::zero())
		{
			self.nextthink = duration_cast<gtime>(level.framenum + self.moveinfo.wait);
			self.think = SAVABLE(train_next);
		}
		else if (self.spawnflags & TRAIN_TOGGLE)
		{ // && wait < 0
#ifdef GROUND_ZERO
			self.target_ent = 0;
#else
			train_next(self);
#endif
			self.spawnflags &= ~TRAIN_START_ON;
			self.velocity = vec3_origin;
			self.nextthink = gtime::zero();
		}

		if (!(self.flags & FL_TEAMSLAVE))
		{
			if (self.moveinfo.sound_end)
				gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_end, ATTN_STATIC);
			self.sound = SOUND_NONE;
		}
	}
	else
		train_next(self);
}

REGISTER_STATIC_SAVABLE(train_wait);

#ifdef GROUND_ZERO
static cvarref g_legacy_trains;

static void train_piece_wait(entity &)
{
}

REGISTER_STATIC_SAVABLE(train_piece_wait);
#endif

static void train_next(entity &self)
{
	bool first = true;

again:
	if (!self.target)
		return;

	entityref ent = G_PickTarget(self.target);
	if (!ent.has_value())
	{
		gi.dprintfmt("train_next: bad target {}\n", self.target);
		return;
	}

	self.target = ent->target;

	// check for a teleport path_corner
	if (ent->spawnflags & TRAIN_START_ON)
	{
		if (!first)
		{
			gi.dprintfmt("{}: connected teleport path_corners\n", ent);
			return;
		}
		first = false;
		self.origin = ent->origin - self.bounds.mins;
		self.old_origin = self.origin;
		self.event = EV_OTHER_TELEPORT;
		gi.linkentity(self);
		goto again;
	}

#ifdef GROUND_ZERO
	if (ent->speed)
	{
		self.speed = ent->speed;
		self.moveinfo.speed = ent->speed;
		if(ent->accel)
			self.moveinfo.accel = ent->accel;
		else
			self.moveinfo.accel = ent->speed;
		if(ent->decel)
			self.moveinfo.decel = ent->decel;
		else
			self.moveinfo.decel = ent->speed;
		self.moveinfo.current_speed = 0;
	}
#endif

	self.moveinfo.wait = ent->wait;
	self.target_ent = ent;

	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_start)
			gi.sound(self, CHAN_VOICE | CHAN_NO_PHS_ADD, self.moveinfo.sound_start, ATTN_STATIC);
		self.sound = self.moveinfo.sound_middle;
	}

	vector dest = ent->origin - self.bounds.mins;
	self.moveinfo.state = STATE_TOP;
	self.moveinfo.start_origin = self.origin;
	self.moveinfo.end_origin = dest;
	Move_Calc(self, dest, SAVABLE(train_wait));
	self.spawnflags |= TRAIN_START_ON;

#ifdef GROUND_ZERO
	if(self.team && !g_legacy_trains)
	{
		vector dir = dest - self.origin;

		for (entityref e = self.teamchain; e.has_value(); e = e->teamchain)
		{
			vector dst = dir + e->origin;

			e->moveinfo.start_origin = e->origin;
			e->moveinfo.end_origin = dst;

			e->moveinfo.state = STATE_TOP;
			e->speed = self.speed;
			e->moveinfo.speed = self.moveinfo.speed;
			e->moveinfo.accel = self.moveinfo.accel;
			e->moveinfo.decel = self.moveinfo.decel;
			e->movetype = MOVETYPE_PUSH;
			Move_Calc (e, dst, SAVABLE(train_piece_wait));
		}
	
	}
#endif
}

static void train_resume(entity &self)
{
	entity &ent = self.target_ent;
	vector dest = ent.origin - self.bounds.mins;
	self.moveinfo.state = STATE_TOP;
	self.moveinfo.start_origin = self.origin;
	self.moveinfo.end_origin = dest;
	Move_Calc(self, dest, SAVABLE(train_wait));
	self.spawnflags |= TRAIN_START_ON;
}

void func_train_find(entity &self)
{
	if (!self.target)
	{
		gi.dprintfmt("{}: no target\n", self);
		return;
	}

	entityref ent = G_PickTarget(self.target);

	if (!ent.has_value())
	{
		gi.dprintfmt("{}: target \"{}\" not found\n", self, self.target);
		return;
	}

	self.target = ent->target;

	self.origin = ent->origin - self.bounds.mins;
	gi.linkentity(self);

	// if not triggered, start immediately
	if (!self.targetname)
		self.spawnflags |= TRAIN_START_ON;

	if (self.spawnflags & TRAIN_START_ON)
	{
		self.nextthink = level.framenum + 1_hz;
		self.think = SAVABLE(train_next);
		self.activator = self;
	}
}

REGISTER_SAVABLE(func_train_find);

void train_use(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;

	if (self.spawnflags & TRAIN_START_ON)
	{
		if (!(self.spawnflags & TRAIN_TOGGLE))
			return;
		self.spawnflags &= ~TRAIN_START_ON;
		self.velocity = vec3_origin;
		self.nextthink = gtime::zero();
	}
	else if (self.target_ent.has_value())
		train_resume(self);
	else
		train_next(self);
}

REGISTER_SAVABLE(train_use);

static void SP_func_train(entity &self)
{
#ifdef GROUND_ZERO
	g_legacy_trains = gi.cvar("g_legacy_trains", "0", CVAR_LATCH);
#endif

	self.movetype = MOVETYPE_PUSH;

	self.angles = vec3_origin;
	self.blocked = SAVABLE(train_blocked);
	if (self.spawnflags & TRAIN_BLOCK_STOPS)
		self.dmg = 0;
	else if (!self.dmg)
		self.dmg = 100;
	self.solid = SOLID_BSP;
	gi.setmodel(self, self.model);

	if (st.noise)
		self.moveinfo.sound_middle = gi.soundindex(st.noise);

	if (!self.speed)
		self.speed = 100.f;

	self.moveinfo.accel = self.moveinfo.decel = self.moveinfo.speed = self.speed;

	self.use = SAVABLE(train_use);

	gi.linkentity(self);

	if (self.target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self.nextthink = level.framenum + 1_hz;
		self.think = SAVABLE(func_train_find);
	}
	else
		gi.dprintfmt("{}: no target\n", self);
}

REGISTER_ENTITY(FUNC_TRAIN, func_train);

/*QUAKED trigger_elevator (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
*/
static void trigger_elevator_use(entity &self, entity &other, entity &)
{
	if (self.movetarget->nextthink != gtime::zero())
		return;

	if (!other.pathtarget)
	{
		gi.dprintfmt("{}: used with no pathtarget\n", self);
		return;
	}

	entityref target = G_PickTarget(other.pathtarget);

	if (!target.has_value())
	{
		gi.dprintfmt("{}: used with bad pathtarget \"{}\"\n", self, other.pathtarget);
		return;
	}

	self.movetarget->target_ent = target;
	train_resume(self.movetarget);
}

REGISTER_STATIC_SAVABLE(trigger_elevator_use);

static void trigger_elevator_init(entity &self)
{
	if (!self.target)
	{
		gi.dprintfmt("{}: no target\n", self);
		return;
	}

	self.movetarget = G_PickTarget(self.target);

	if (!self.movetarget.has_value())
	{
		gi.dprintfmt("{}: unable to find target \"{}\"\n", self, self.target);
		return;
	}

	if (self.movetarget->type != ET_FUNC_TRAIN)
	{
		gi.dprintfmt("{}: target \"{}\" is not a train\n", self, self.movetarget);
		return;
	}

	self.use = SAVABLE(trigger_elevator_use);
	self.svflags = SVF_NOCLIENT;
}

REGISTER_STATIC_SAVABLE(trigger_elevator_init);

static void SP_trigger_elevator(entity &self)
{
	self.think = SAVABLE(trigger_elevator_init);
	self.nextthink = level.framenum + 1_hz;
}

static REGISTER_ENTITY(TRIGGER_ELEVATOR, trigger_elevator);

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
"wait"          base time between triggering all targets, default is 1
"random"        wait variance, default is 0

so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"delay"         delay before first firing when turned on, default is 0

"pausetime"     additional delay used only the very first time
				and only if spawned with START_ON

These can used but not touched.
*/
static void func_timer_think(entity &self)
{
	G_UseTargets(self, self.activator);
	self.nextthink = duration_cast<gtime>(level.framenum + self.wait + random(-self.rand, self.rand));
}

REGISTER_STATIC_SAVABLE(func_timer_think);

static void func_timer_use(entity &self, entity &, entity &cactivator)
{
	self.activator = cactivator;

	// if on, turn it off
	if (self.nextthink != gtime::zero())
	{
		self.nextthink = gtime::zero();
		return;
	}

	// turn it on
	if (self.delay != gtimef::zero())
		self.nextthink = duration_cast<gtime>(level.framenum + self.delay);
	else
		func_timer_think(self);
}

REGISTER_STATIC_SAVABLE(func_timer_use);

static constexpr spawn_flag TIMER_START_ON = (spawn_flag) 1;

static void SP_func_timer(entity &self)
{
	if (self.wait == gtimef::zero())
		self.wait = 1s;

	self.use = SAVABLE(func_timer_use);
	self.think = SAVABLE(func_timer_think);

	if (self.rand >= self.wait)
	{
		self.rand = self.wait - FRAMETIME;
		gi.dprintfmt("{}: random >= wait\n", self);
	}

	if (self.spawnflags & TIMER_START_ON)
	{
		self.nextthink = duration_cast<gtime>(level.framenum + 1s + st.pausetime + self.delay + self.wait + random(-self.rand, self.rand));
		self.activator = self;
	}

	self.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(FUNC_TIMER, func_timer);

/*QUAKED func_conveyor (0 .5 .8) ? START_ON TOGGLE
Conveyors are stationary brushes that move what's on them.
The brush should be have a surface with at least one current content enabled.
speed   default 100
*/
constexpr spawn_flag CONVEYOR_START_ON = (spawn_flag) 1;
constexpr spawn_flag CONVEYOR_TOGGLE = (spawn_flag) 2;

static void func_conveyor_use(entity &self, entity &, entity &)
{
	if (self.spawnflags & CONVEYOR_START_ON)
	{
		self.speed = 0;
		self.spawnflags &= ~CONVEYOR_START_ON;
	}
	else
	{
		self.speed = (float)self.count;
		self.spawnflags |= CONVEYOR_START_ON;
	}

	if (!(self.spawnflags & CONVEYOR_TOGGLE))
		self.count = 0;
}

REGISTER_STATIC_SAVABLE(func_conveyor_use);

static void SP_func_conveyor(entity &self)
{
	if (!self.speed)
		self.speed = 100.f;

	if (!(self.spawnflags & CONVEYOR_START_ON))
	{
		self.count = (int32_t)self.speed;
		self.speed = 0;
	}

	self.use = SAVABLE(func_conveyor_use);

	gi.setmodel(self, self.model);
	self.solid = SOLID_BSP;
	gi.linkentity(self);
}

static REGISTER_ENTITY(FUNC_CONVEYOR, func_conveyor);

/*QUAKED func_door_secret (0 .5 .8) ? always_shoot 1st_left 1st_down
A secret door.  Slide back and then to the side.

open_once       doors never closes
1st_left        1st move is left of arrow
1st_down        1st move is down from arrow
always_shoot    door is shootebale even if targeted

"angle"     determines the direction
"dmg"       damage to inflic when blocked (default 2)
"wait"      how long to hold in the open position (default 5, -1 means hold)
*/

constexpr spawn_flag SECRET_ALWAYS_SHOOT	= (spawn_flag)1;
constexpr spawn_flag SECRET_1ST_LEFT		= (spawn_flag)2;
constexpr spawn_flag SECRET_1ST_DOWN		= (spawn_flag)4;

static void door_secret_move1(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move1);

static void door_secret_move2(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move2);

static void door_secret_move3(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move3);

static void door_secret_move4(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move4);

static void door_secret_move5(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move5);

static void door_secret_move6(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_move6);

static void door_secret_done(entity &self);

REGISTER_STATIC_SAVABLE(door_secret_done);

static void door_secret_use(entity &self, entity &, entity &)
{
	// make sure we're not already moving
	if (self.origin)
		return;

	Move_Calc(self, self.pos1, SAVABLE(door_secret_move1));
	door_use_areaportals(self, true);
}

REGISTER_STATIC_SAVABLE(door_secret_use);

static void door_secret_move1(entity &self)
{
	self.nextthink = level.framenum + 1s;
	self.think = SAVABLE(door_secret_move2);
}

static void door_secret_move2(entity &self)
{
	Move_Calc(self, self.pos2, SAVABLE(door_secret_move3));
}

static void door_secret_move3(entity &self)
{
	if (self.wait == -1s)
		return;
	self.nextthink = duration_cast<gtime>(level.framenum + self.wait);
	self.think = SAVABLE(door_secret_move4);
}

static void door_secret_move4(entity &self)
{
	Move_Calc(self, self.pos1, SAVABLE(door_secret_move5));
}

static void door_secret_move5(entity &self)
{
	self.nextthink = level.framenum + 1s;
	self.think = SAVABLE(door_secret_move6);
}

static void door_secret_move6(entity &self)
{
	Move_Calc(self, vec3_origin, SAVABLE(door_secret_done));
}

static void door_secret_done(entity &self)
{
	if (!self.targetname || (self.spawnflags & SECRET_ALWAYS_SHOOT))
	{
		self.health = 0;
		self.takedamage = true;
	}
	door_use_areaportals(self, false);
}

static void door_secret_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	if (level.framenum < self.touch_debounce_framenum)
		return;

	self.touch_debounce_framenum = level.framenum + 500ms;

	T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(door_secret_blocked);

static void door_secret_die(entity &self, entity &, entity &attacker, int32_t, vector)
{
	self.takedamage = false;
	door_secret_use(self, attacker, attacker);
}

REGISTER_STATIC_SAVABLE(door_secret_die);

static void SP_func_door_secret(entity &ent)
{
	ent.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
	ent.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
	ent.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");

	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.model);

	ent.blocked = SAVABLE(door_secret_blocked);
	ent.use = SAVABLE(door_secret_use);

	if (!ent.targetname || (ent.spawnflags & SECRET_ALWAYS_SHOOT))
	{
		ent.health = 0;
		ent.takedamage = true;
		ent.die = SAVABLE(door_secret_die);
	}

	if (!ent.dmg)
		ent.dmg = 2;

	if (ent.wait == gtimef::zero())
		ent.wait = 5s;

	ent.moveinfo.accel =
		ent.moveinfo.decel =
			ent.moveinfo.speed = 50.f;

	// calculate positions
	vector forward, right, up;
	AngleVectors(ent.angles, &forward, &right, &up);
	ent.angles = vec3_origin;

	float side = 1.0f - (float)(ent.spawnflags & SECRET_1ST_LEFT);
	float width;

	if (ent.spawnflags & SECRET_1ST_DOWN)
		width = fabs(up * ent.size);
	else
		width = fabs(right * ent.size);

	float length = fabs(forward * ent.size);

	if (ent.spawnflags & SECRET_1ST_DOWN)
		ent.pos1 = ent.origin + ((-1 * width) * up);
	else
		ent.pos1 = ent.origin + ((side * width) * right);
	ent.pos2 = ent.pos1 + (length * forward);

	if (ent.health)
	{
		ent.takedamage = true;
		ent.die = SAVABLE(door_killed);
		ent.max_health = ent.health;
	}
	else if (ent.targetname && ent.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.touch = SAVABLE(door_touch);
	}

	gi.linkentity(ent);
}

static REGISTER_ENTITY_INHERIT(FUNC_DOOR_SECRET, func_door_secret, ET_FUNC_DOOR);

/*QUAKED func_killbox (1 0 0) ?
Kills everything inside when fired, irrespective of protection.
*/
static void use_killbox(entity &self, entity &, entity &)
{
	KillBox(self);
}

REGISTER_STATIC_SAVABLE(use_killbox);

static void SP_func_killbox(entity &ent)
{
	gi.setmodel(ent, ent.model);
	ent.use = SAVABLE(use_killbox);
	ent.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(FUNC_KILLBOX, func_killbox);
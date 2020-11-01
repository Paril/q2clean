#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "func.h"
#include "game.h"
#include "combat.h"
#include "util.h"
#include "spawn.h"

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
constexpr spawn_flag DOOR_INACTIVE		= (spawn_flag)8192;

//
// Support routines for movement (changes in origin using velocity)
//

static void Move_Done(entity &ent)
{
	ent.g.velocity = vec3_origin;
	ent.g.moveinfo.endfunc(ent);
}

static void Move_Final(entity &ent)
{
	if (ent.g.moveinfo.remaining_distance == 0)
	{
		Move_Done(ent);
		return;
	}

	ent.g.velocity = ent.g.moveinfo.dir * (ent.g.moveinfo.remaining_distance / FRAMETIME);

	ent.g.think = Move_Done;
	ent.g.nextthink = level.framenum + 1;
}

static void Move_Begin(entity &ent)
{
	if ((ent.g.moveinfo.speed * FRAMETIME) >= ent.g.moveinfo.remaining_distance)
	{
		Move_Final(ent);
		return;
	}

	ent.g.velocity = ent.g.moveinfo.dir * ent.g.moveinfo.speed;
	float frames = floor((ent.g.moveinfo.remaining_distance / ent.g.moveinfo.speed) / FRAMETIME);
	ent.g.moveinfo.remaining_distance -= frames * ent.g.moveinfo.speed * FRAMETIME;
	ent.g.nextthink = level.framenum + (gtime)frames;
	ent.g.think = Move_Final;
}

static void Think_AccelMove(entity &ent);
static void plat_CalcAcceleratedMove(entity &ent);

void Move_Calc(entity &ent, vector dest, thinkfunc func)
{
	ent.g.velocity = vec3_origin;
	ent.g.moveinfo.dir = dest - ent.s.origin;
	ent.g.moveinfo.remaining_distance = VectorNormalize(ent.g.moveinfo.dir);
	ent.g.moveinfo.endfunc = func;

	if (ent.g.moveinfo.speed == ent.g.moveinfo.accel && ent.g.moveinfo.speed == ent.g.moveinfo.decel)
	{
		if (level.current_entity == ((ent.g.flags & FL_TEAMSLAVE) ? (entity &)ent.g.teammaster : ent))
			Move_Begin(ent);
		else
		{
			ent.g.nextthink = level.framenum + 1;
			ent.g.think = Move_Begin;
		}
	}
	else
	{
		// accelerative
		plat_CalcAcceleratedMove(ent);
		
		ent.g.moveinfo.current_speed = 0;
		ent.g.think = Think_AccelMove;
		ent.g.nextthink = level.framenum + 1;
	}
}

//
// Support routines for angular movement (changes in angle using avelocity)
//

static void AngleMove_Done(entity &ent)
{
	ent.g.avelocity = vec3_origin;
	ent.g.moveinfo.endfunc(ent);
}

static void AngleMove_Final(entity &ent)
{
	vector	move;

	if (ent.g.moveinfo.state == STATE_UP)
		move = ent.g.moveinfo.end_angles - ent.s.angles;
	else
		move = ent.g.moveinfo.start_angles - ent.s.angles;

	if (!move)
	{
		AngleMove_Done(ent);
		return;
	}

	ent.g.avelocity = move * (1.0f / FRAMETIME);

	ent.g.think = AngleMove_Done;
	ent.g.nextthink = level.framenum + 1;
}

static void AngleMove_Begin(entity &ent)
{
	vector	destdelta;
	float	len;
	float	traveltime;
	float	frames;

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
	if (ent.g.moveinfo.state == STATE_UP)
		destdelta = ent.g.moveinfo.end_angles - ent.s.angles;
	else
		destdelta = ent.g.moveinfo.start_angles - ent.s.angles;

	// calculate length of vector
	len = VectorLength(destdelta);

	// divide by speed to get time to reach dest
	traveltime = len / ent.g.moveinfo.speed;

	if (traveltime < FRAMETIME)
	{
		AngleMove_Final(ent);
		return;
	}

	frames = floor(traveltime / FRAMETIME);

	// scale the destdelta vector by the time spent traveling to get velocity
	ent.g.avelocity = destdelta * (1.0f / traveltime);

#ifdef GROUND_ZERO
	// if we're done accelerating, act as a normal rotation
	if(ent.moveinfo.speed >= ent.speed)
	{
#endif
		// set nextthink to trigger a think when dest is reached
		ent.g.nextthink = level.framenum + (gtime)frames;
		ent.g.think = AngleMove_Final;
#ifdef GROUND_ZERO
	}
	else
	{
		ent.nextthink = level.framenum + 1;
		ent.think = AngleMove_Begin;
	}
#endif
}

static void AngleMove_Calc(entity &ent, thinkfunc func)
{
	ent.g.avelocity = vec3_origin;
	ent.g.moveinfo.endfunc = func;
	
#ifdef GROUND_ZERO
	// if we're supposed to accelerate, this will tell anglemove_begin to do so
	if(ent.accel != ent.speed)
		ent.moveinfo.speed = 0;
#endif
	
	if (level.current_entity == ((ent.g.flags & FL_TEAMSLAVE) ? (entity &)ent.g.teammaster : ent))
		AngleMove_Begin(ent);
	else
	{
		ent.g.nextthink = level.framenum + 1;
		ent.g.think = AngleMove_Begin;
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
	ent.g.moveinfo.move_speed = ent.g.moveinfo.speed;

	if (ent.g.moveinfo.remaining_distance < ent.g.moveinfo.accel)
	{
		ent.g.moveinfo.current_speed = ent.g.moveinfo.remaining_distance;
		return;
	}

	float accel_dist = AccelerationDistance(ent.g.moveinfo.speed, ent.g.moveinfo.accel);
	float decel_dist = AccelerationDistance(ent.g.moveinfo.speed, ent.g.moveinfo.decel);

	if ((ent.g.moveinfo.remaining_distance - accel_dist - decel_dist) < 0)
	{
		float f = (ent.g.moveinfo.accel + ent.g.moveinfo.decel) / (ent.g.moveinfo.accel * ent.g.moveinfo.decel);
		ent.g.moveinfo.move_speed = (-2 + sqrt(4 - 4 * f * (-2 * ent.g.moveinfo.remaining_distance))) / (2 * f);
		decel_dist = AccelerationDistance(ent.g.moveinfo.move_speed, ent.g.moveinfo.decel);
	}

	ent.g.moveinfo.decel_distance = decel_dist;
}

static void plat_Accelerate(entity &ent)
{
	// are we decelerating?
	if (ent.g.moveinfo.remaining_distance <= ent.g.moveinfo.decel_distance)
	{
		if (ent.g.moveinfo.remaining_distance < ent.g.moveinfo.decel_distance)
		{
			if (ent.g.moveinfo.next_speed)
			{
				ent.g.moveinfo.current_speed = ent.g.moveinfo.next_speed;
				ent.g.moveinfo.next_speed = 0;
				return;
			}
			if (ent.g.moveinfo.current_speed > ent.g.moveinfo.decel)
				ent.g.moveinfo.current_speed -= ent.g.moveinfo.decel;
		}
		return;
	}

	// are we at full speed and need to start decelerating during this move?
	if (ent.g.moveinfo.current_speed == ent.g.moveinfo.move_speed)
		if ((ent.g.moveinfo.remaining_distance - ent.g.moveinfo.current_speed) < ent.g.moveinfo.decel_distance)
		{
			float p1_distance = ent.g.moveinfo.remaining_distance - ent.g.moveinfo.decel_distance;
			float p2_distance = ent.g.moveinfo.move_speed * (1.0f - (p1_distance / ent.g.moveinfo.move_speed));
			float distance = p1_distance + p2_distance;
			ent.g.moveinfo.current_speed = ent.g.moveinfo.move_speed;
			ent.g.moveinfo.next_speed = ent.g.moveinfo.move_speed - ent.g.moveinfo.decel * (p2_distance / distance);
			return;
		}

	// are we accelerating?
	if (ent.g.moveinfo.current_speed < ent.g.moveinfo.speed)
	{
		float old_speed = ent.g.moveinfo.current_speed;

		// figure simple acceleration up to move_speed
		ent.g.moveinfo.current_speed += ent.g.moveinfo.accel;
		if (ent.g.moveinfo.current_speed > ent.g.moveinfo.speed)
			ent.g.moveinfo.current_speed = ent.g.moveinfo.speed;

		// are we accelerating throughout this entire move?
		if ((ent.g.moveinfo.remaining_distance - ent.g.moveinfo.current_speed) >= ent.g.moveinfo.decel_distance)
			return;

		// during this move we will accelrate from current_speed to move_speed
		// and cross over the decel_distance; figure the average speed for the
		// entire move
		float p1_distance = ent.g.moveinfo.remaining_distance - ent.g.moveinfo.decel_distance;
		float p1_speed = (old_speed + ent.g.moveinfo.move_speed) / 2.0f;
		float p2_distance = ent.g.moveinfo.move_speed * (1.0f - (p1_distance / p1_speed));
		float distance = p1_distance + p2_distance;
		ent.g.moveinfo.current_speed = (p1_speed * (p1_distance / distance)) + (ent.g.moveinfo.move_speed * (p2_distance / distance));
		ent.g.moveinfo.next_speed = ent.g.moveinfo.move_speed - ent.g.moveinfo.decel * (p2_distance / distance);
		return;
	}

	// we are at constant velocity (move_speed)
}

static void Think_AccelMove(entity &ent)
{
	ent.g.moveinfo.remaining_distance -= ent.g.moveinfo.current_speed;

	plat_Accelerate(ent);

	// will the entire move complete on next frame?
	if (ent.g.moveinfo.remaining_distance <= ent.g.moveinfo.current_speed)
	{
		Move_Final(ent);
		return;
	}

	ent.g.velocity = ent.g.moveinfo.dir * (ent.g.moveinfo.current_speed * 10);
	ent.g.nextthink = level.framenum + 1;
	ent.g.think = Think_AccelMove;
}

static void plat_go_down(entity &ent);

static void plat_hit_top(entity &ent)
{
	if (!(ent.g.flags & FL_TEAMSLAVE))
	{
		if (ent.g.moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.g.moveinfo.sound_end, 1, ATTN_STATIC, 0);
		ent.s.sound = SOUND_NONE;
	}
	ent.g.moveinfo.state = STATE_TOP;

	ent.g.think = plat_go_down;
	ent.g.nextthink = level.framenum + 3 * BASE_FRAMERATE;
}

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
void(entity ent) plat2_kill_danger_area;
void(entity ent) plat2_spawn_danger_area;
#endif

static void plat_hit_bottom(entity &ent)
{
	if (!(ent.g.flags & FL_TEAMSLAVE))
	{
		if (ent.g.moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.g.moveinfo.sound_end, 1, ATTN_STATIC, 0);
		ent.s.sound = SOUND_NONE;
	}
	ent.g.moveinfo.state = STATE_BOTTOM;
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)

	plat2_kill_danger_area (ent);
#endif
}

static void plat_go_down(entity &ent)
{
	if (!(ent.g.flags & FL_TEAMSLAVE))
	{
		if (ent.g.moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		ent.s.sound = ent.g.moveinfo.sound_middle;
	}
	ent.g.moveinfo.state = STATE_DOWN;
	Move_Calc(ent, ent.g.moveinfo.end_origin, plat_hit_bottom);
#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)

	plat2_spawn_danger_area(ent);
#endif
}

static void plat_go_up(entity &ent)
{
	if (!(ent.g.flags & FL_TEAMSLAVE))
	{
		if (ent.g.moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		ent.s.sound = ent.g.moveinfo.sound_middle;
	}
	ent.g.moveinfo.state = STATE_UP;
	Move_Calc(ent, ent.g.moveinfo.start_origin, plat_hit_top);
}

static void plat_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

#ifdef GROUND_ZERO
	// gib dead things
	if(other.health < 1)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 100, 1, 0, MOD_CRUSH);
#endif

	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);

	if (self.g.moveinfo.state == STATE_UP)
		plat_go_down(self);
	else if (self.g.moveinfo.state == STATE_DOWN)
		plat_go_up(self);
}

void Use_Plat(entity &ent, entity &other [[maybe_unused]], entity &)
{
#ifdef GROUND_ZERO
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
	
	if (ent.g.think)
		return;     // already down
	plat_go_down(ent);
}

static void Touch_Plat_Center(entity &ent, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (other.g.health <= 0)
		return;

	entity &plat = ent.g.enemy;   // now point at the plat, not the trigger

	if (plat.g.moveinfo.state == STATE_BOTTOM)
		plat_go_up(plat);
	else if (plat.g.moveinfo.state == STATE_TOP)
		plat.g.nextthink = level.framenum + 1 * BASE_FRAMERATE;   // the player is still on the plat, so delay going down
}

entity &plat_spawn_inside_trigger(entity &ent)
{
//
// middle trigger
//
	entity &trigger = G_Spawn();
	trigger.g.touch = Touch_Plat_Center;
	trigger.g.movetype = MOVETYPE_NONE;
	trigger.solid = SOLID_TRIGGER;
	trigger.g.enemy = ent;

	vector tmin, tmax;

	tmin.x = ent.mins.x + 25;
	tmin.y = ent.mins.y + 25;
	tmin.z = ent.mins.z;

	tmax.x = ent.maxs.x - 25;
	tmax.y = ent.maxs.y - 25;
	tmax.z = ent.maxs.z + 8;

	tmin.z = tmax.z - (ent.g.pos1.z - ent.g.pos2.z + st.lip);

	if (ent.g.spawnflags & PLAT_LOW_TRIGGER)
		tmax.z = tmin.z + 8;

	if (tmax.x - tmin.x <= 0)
	{
		tmin.x = (ent.mins.x + ent.maxs.x) * 0.5f;
		tmax.x = tmin.x + 1;
	}
	if (tmax.y - tmin.y <= 0)
	{
		tmin.y = (ent.mins.y + ent.maxs.y) * 0.5f;
		tmax.y = tmin.y + 1;
	}

	trigger.mins = tmin;
	trigger.maxs = tmax;

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
	ent.s.angles = vec3_origin;
	ent.solid = SOLID_BSP;
	ent.g.movetype = MOVETYPE_PUSH;

	gi.setmodel(ent, ent.g.model);

	ent.g.blocked = plat_blocked;

	if (!ent.g.speed)
		ent.g.speed = 20.f;
	else
		ent.g.speed *= 0.1f;

	if (!ent.g.accel)
		ent.g.accel = 5.f;
	else
		ent.g.accel *= 0.1f;

	if (!ent.g.decel)
		ent.g.decel = 5.f;
	else
		ent.g.decel *= 0.1f;

	if (!ent.g.dmg)
		ent.g.dmg = 2;

	if (!st.lip)
		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	ent.g.pos1 = ent.s.origin;
	ent.g.pos2 = ent.s.origin;
	if (st.height)
		ent.g.pos2.z -= st.height;
	else
		ent.g.pos2.z -= (ent.maxs.z - ent.mins.z) - st.lip;

	ent.g.use = Use_Plat;

	plat_spawn_inside_trigger(ent);     // the "start moving" trigger

	if (ent.g.targetname)
		ent.g.moveinfo.state = STATE_UP;
	else
	{
		ent.s.origin = ent.g.pos2;
		gi.linkentity(ent);
		ent.g.moveinfo.state = STATE_BOTTOM;
	}

	ent.g.moveinfo.speed = ent.g.speed;
	ent.g.moveinfo.accel = ent.g.accel;
	ent.g.moveinfo.decel = ent.g.decel;
	ent.g.moveinfo.wait = ent.g.wait;
	ent.g.moveinfo.start_origin = ent.g.pos1;
	ent.g.moveinfo.start_angles = ent.s.angles;
	ent.g.moveinfo.end_origin = ent.g.pos2;
	ent.g.moveinfo.end_angles = ent.s.angles;

	ent.g.moveinfo.sound_start = gi.soundindex("plats/pt1_strt.wav");
	ent.g.moveinfo.sound_middle = gi.soundindex("plats/pt1_mid.wav");
	ent.g.moveinfo.sound_end = gi.soundindex("plats/pt1_end.wav");
}

REGISTER_ENTITY(func_plat, ET_FUNC_PLAT);

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
static void(entity self) rotating_accel =
{
	float	current_speed;
	current_speed = VectorLength (self.avelocity);

	if (current_speed >= (self.speed - self.accel))		// done
	{
		self.avelocity = self.movedir * self.speed;
		G_UseTargets (self, self);
	}
	else
	{
		current_speed += self.accel;
		self.avelocity = self.movedir * current_speed;
		self.think = rotating_accel;
		self.nextthink = level.framenum + 1;
	}
}

static void(entity self) rotating_decel =
{
	float	current_speed;

	current_speed = VectorLength (self.avelocity);
	if(current_speed <= self.decel)		// done
	{
		self.avelocity = vec3_origin;
		G_UseTargets (self, self);
		self.touch = 0;
	}
	else
	{
		current_speed -= self.decel;
		self.avelocity = self.movedir * current_speed;
		self.think = rotating_decel;
		self.nextthink = level.framenum + 1;
	}
}
#endif

static void rotating_blocked(entity &self, entity &other)
{
	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, (int32_t)self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

static void rotating_touch(entity &self, entity &other, vector, const surface &)
{
	if (self.g.avelocity)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, (int32_t)self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

static void rotating_use(entity &self, entity &, entity &)
{
	if (self.g.avelocity)
	{
		self.s.sound = SOUND_NONE;
#ifdef GROUND_ZERO
		if(self.spawnflags & 8192)	// Decelerate
			rotating_decel (self);
		else
		{
			G_UseTargets (self, self);
#endif
			self.g.avelocity = vec3_origin;
			self.g.touch = 0;
#ifdef GROUND_ZERO
		}
#endif
	}
	else
	{
		self.s.sound = self.g.moveinfo.sound_middle;

#ifdef GROUND_ZERO
		if(self.spawnflags & 8192)	// accelerate
			rotating_accel (self);
		else
		{
			G_UseTargets (self, self);
#endif
			self.g.avelocity = self.g.movedir * self.g.speed;
#ifdef GROUND_ZERO
		}
#endif
		if (self.g.spawnflags & ROTATING_TOUCH_PAIN)
			self.g.touch = rotating_touch;
	}
}

static void SP_func_rotating(entity &ent)
{
	ent.solid = SOLID_BSP;
	if (ent.g.spawnflags & ROTATING_STOP)
		ent.g.movetype = MOVETYPE_STOP;
	else
		ent.g.movetype = MOVETYPE_PUSH;

	// set the axis of rotation
	ent.g.movedir = vec3_origin;
	if (ent.g.spawnflags & ROTATING_X_AXIS)
		ent.g.movedir.z = 1.0f;
	else if (ent.g.spawnflags & ROTATING_Y_AXIS)
		ent.g.movedir.x = 1.0f;
	else // Z_AXIS
		ent.g.movedir.y = 1.0f;

	// check for reverse rotation
	if (ent.g.spawnflags & ROTATING_REVERSE)
		ent.g.movedir = -ent.g.movedir;

	if (!ent.g.speed)
		ent.g.speed = 100.f;
	if (!ent.g.dmg)
		ent.g.dmg = 2;

	ent.g.use = rotating_use;
	if (ent.g.dmg)
		ent.g.blocked = rotating_blocked;

	if (ent.g.spawnflags & ROTATING_START_ON)
		ent.g.use(ent, world, world);

	if (ent.g.spawnflags & ROTATING_ANIMATED)
		ent.s.effects |= EF_ANIM_ALL;
	if (ent.g.spawnflags & ROTATING_ANIMATED_FAST)
		ent.s.effects |= EF_ANIM_ALLFAST;

#ifdef GROUND_ZERO
	if(ent.spawnflags & 8192)	// Accelerate / Decelerate
	{
		if(!ent.accel)
			ent.accel = 1f;
		else if (ent.accel > ent.speed)
			ent.accel = ent.speed;

		if(!ent.decel)
			ent.decel = 1f;
		else if (ent.decel > ent.speed)
			ent.decel = ent.speed;
	}
#endif

	gi.setmodel(ent, ent.g.model);
	gi.linkentity(ent);
}

REGISTER_ENTITY(func_rotating, ET_FUNC_ROTATING);

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
	self.g.moveinfo.state = STATE_BOTTOM;
	self.s.effects &= ~EF_ANIM23;
	self.s.effects |= EF_ANIM01;
}

static void button_return(entity &self)
{
	self.g.moveinfo.state = STATE_DOWN;

	Move_Calc(self, self.g.moveinfo.start_origin, button_done);

	self.s.frame = 0;

	if (self.g.health)
		self.g.takedamage = true;
}

static void button_wait(entity &self)
{
	self.g.moveinfo.state = STATE_TOP;
	self.s.effects &= ~EF_ANIM01;
	self.s.effects |= EF_ANIM23;

	G_UseTargets(self, self.g.activator);
	self.s.frame = 1;
	if (self.g.moveinfo.wait >= 0)
	{
		self.g.nextthink = level.framenum + (gtime)(self.g.moveinfo.wait * BASE_FRAMERATE);
		self.g.think = button_return;
	}
}

static void button_fire(entity &self)
{
	if (self.g.moveinfo.state == STATE_UP || self.g.moveinfo.state == STATE_TOP)
		return;

	self.g.moveinfo.state = STATE_UP;
	if (self.g.moveinfo.sound_start && !(self.g.flags & FL_TEAMSLAVE))
		gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
	Move_Calc(self, self.g.moveinfo.end_origin, button_wait);
}

static void button_use(entity &self, entity &, entity &cactivator)
{
	self.g.activator = cactivator;
	button_fire(self);
}

static void button_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (other.g.health <= 0)
		return;

	self.g.activator = other;
	button_fire(self);
}

static void button_killed(entity &self, entity &, entity &attacker, int32_t, vector)
{
	self.g.activator = attacker;
	self.g.health = self.g.max_health;
	self.g.takedamage = false;
	button_fire(self);
}

static void SP_func_button(entity &ent)
{
	G_SetMovedir(ent.s.angles, ent.g.movedir);
	ent.g.movetype = MOVETYPE_STOP;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.g.model);

	if (ent.g.sounds != 1)
		ent.g.moveinfo.sound_start = gi.soundindex("switches/butn2.wav");

	if (!ent.g.speed)
		ent.g.speed = 40.f;
	if (!ent.g.accel)
		ent.g.accel = ent.g.speed;
	if (!ent.g.decel)
		ent.g.decel = ent.g.speed;

	if (!ent.g.wait)
		ent.g.wait = 3.f;
	if (!st.lip)
		st.lip = 4;

	ent.g.pos1 = ent.s.origin;
	vector	abs_movedir;
	abs_movedir.x = fabs(ent.g.movedir.x);
	abs_movedir.y = fabs(ent.g.movedir.y);
	abs_movedir.z = fabs(ent.g.movedir.z);
	float dist = abs_movedir.x * ent.size.x + abs_movedir.y * ent.size.y + abs_movedir.z * ent.size.z - st.lip;
	ent.g.pos2 = ent.g.pos1 + (dist * ent.g.movedir);

	ent.g.use = button_use;
	ent.s.effects |= EF_ANIM01;

	if (ent.g.health)
	{
		ent.g.max_health = ent.g.health;
		ent.g.die = button_killed;
		ent.g.takedamage = true;
	}
	else if (!ent.g.targetname)
		ent.g.touch = button_touch;

	ent.g.moveinfo.state = STATE_BOTTOM;

	ent.g.moveinfo.speed = ent.g.speed;
	ent.g.moveinfo.accel = ent.g.accel;
	ent.g.moveinfo.decel = ent.g.decel;
	ent.g.moveinfo.wait = ent.g.wait;
	ent.g.moveinfo.start_origin = ent.g.pos1;
	ent.g.moveinfo.start_angles = ent.s.angles;
	ent.g.moveinfo.end_origin = ent.g.pos2;
	ent.g.moveinfo.end_angles = ent.s.angles;

	gi.linkentity(ent);
}

REGISTER_ENTITY(func_button, ET_FUNC_BUTTON);

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
	if (!self.g.target)
		return;
	
	entityref t;
	while ((t = G_FindFunc(t, g.targetname, self.g.target, striequals)).has_value())
		if (t->g.type == ET_FUNC_AREAPORTAL)
			gi.SetAreaPortalState(t->g.style, open);
}

static void door_go_down(entity &self);

static void door_hit_top(entity &self)
{
	if (!(self.g.flags & FL_TEAMSLAVE))
	{
		if (self.g.moveinfo.sound_end)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_end, 1, ATTN_STATIC, 0);
		self.s.sound = SOUND_NONE;
	}
	
	self.g.moveinfo.state = STATE_TOP;
	
	if (self.g.spawnflags & DOOR_TOGGLE)
		return;

	if (self.g.moveinfo.wait >= 0)
	{
		self.g.think = door_go_down;
		self.g.nextthink = level.framenum + (gtime)(self.g.moveinfo.wait * BASE_FRAMERATE);
	}
}

static void door_hit_bottom(entity &self)
{
	if (!(self.g.flags & FL_TEAMSLAVE))
	{
		if (self.g.moveinfo.sound_end)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_end, 1, ATTN_STATIC, 0);
		self.s.sound = SOUND_NONE;
	}
	self.g.moveinfo.state = STATE_BOTTOM;
	door_use_areaportals(self, false);
}

static void door_go_down(entity &self)
{
	if (!(self.g.flags & FL_TEAMSLAVE))
	{
		if (self.g.moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self.s.sound = self.g.moveinfo.sound_middle;
	}
	if (self.g.max_health)
	{
		self.g.takedamage = true;
		self.g.health = self.g.max_health;
	}

	self.g.moveinfo.state = STATE_DOWN;

	if (self.g.type == ET_FUNC_DOOR)
		Move_Calc(self, self.g.moveinfo.start_origin, door_hit_bottom);
	else if (self.g.type == ET_FUNC_DOOR_ROTATING)
		AngleMove_Calc(self, door_hit_bottom);
}

static void door_go_up(entity &self, entity &cactivator)
{
	if (self.g.moveinfo.state == STATE_UP)
		return;     // already going up

	if (self.g.moveinfo.state == STATE_TOP)
	{
		// reset top wait time
		if (self.g.moveinfo.wait >= 0)
			self.g.nextthink = level.framenum + (gtime)(self.g.moveinfo.wait * BASE_FRAMERATE);
		return;
	}

	if (!(self.g.flags & FL_TEAMSLAVE))
	{
		if (self.g.moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self.s.sound = self.g.moveinfo.sound_middle;
	}

	self.g.moveinfo.state = STATE_UP;

	if (self.g.type == ET_FUNC_DOOR)
		Move_Calc(self, self.g.moveinfo.end_origin, door_hit_top);
	else if (self.g.type == ET_FUNC_DOOR_ROTATING)
		AngleMove_Calc(self, door_hit_top);

	G_UseTargets(self, cactivator);
	door_use_areaportals(self, true);
}

#ifdef GROUND_ZERO
static void(entity self) smart_water_go_up =
{
	float	distance;
	entity	lowestPlayer;
	entity	ent;
	float	lowestPlayerPt;
	int	i;

	if (self.moveinfo.state == STATE_TOP)
	{	// reset top wait time
		if (self.moveinfo.wait >= 0)
			self.nextthink = level.framenum + (int)(self.moveinfo.wait * BASE_FRAMERATE);
		return;
	}

	if (self.health)
	{
		if(self.absmax[2] >= self.health)
		{
			self.velocity = vec3_origin;
			self.nextthink = 0;
			self.moveinfo.state = STATE_TOP;
			return;
		}
	}

	if (!(self.flags & FL_TEAMSLAVE))
	{
		if (self.moveinfo.sound_start)
			gi.sound (self, CHAN_NO_PHS_ADD+CHAN_VOICE, self.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self.s.sound = self.moveinfo.sound_middle;
	}

	// find the lowest player point.
	lowestPlayerPt = FLT_MAX;
	lowestPlayer = world;
	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = itoe(1 + i);

		// don't count dead or unused player slots
		if((ent.inuse) && (ent.health > 0))
		{
			if (ent.absmin[2] < lowestPlayerPt)
			{
				lowestPlayerPt = ent.absmin[2];
				lowestPlayer = ent;
			}
		}
	}

	if !(lowestPlayer)
		return;

	distance = lowestPlayerPt - self.absmax[2];

	// for the calculations, make sure we intend to go up at least a little.
	if(distance < self.accel)
	{
		distance = 100f;
		self.moveinfo.speed = 5f;
	}
	else
		self.moveinfo.speed = distance / self.accel;

	if(self.moveinfo.speed < 5)
		self.moveinfo.speed = 5f;
	else if(self.moveinfo.speed > self.speed)
		self.moveinfo.speed = self.speed;

	// FIXME - should this allow any movement other than straight up?
	self.moveinfo.dir = '0 0 1';	
	self.velocity = self.moveinfo.dir * self.moveinfo.speed;
	self.moveinfo.remaining_distance = distance;

	if(self.moveinfo.state != STATE_UP)
	{
		G_UseTargets (self, lowestPlayer);
		door_use_areaportals (self, true);
		self.moveinfo.state = STATE_UP;
	}

	self.think = smart_water_go_up;
	self.nextthink = level.framenum + 1;
}
#endif

static void door_use(entity &self, entity &, entity &cactivator)
{
	if (self.g.flags & FL_TEAMSLAVE)
		return;

	if (self.g.spawnflags & DOOR_TOGGLE)
	{
		if (self.g.moveinfo.state == STATE_UP || self.g.moveinfo.state == STATE_TOP)
		{
			// trigger all paired doors
			for (entityref ent = self; ent.has_value(); ent = ent->g.teamchain)
			{
				ent->g.message = nullptr;
				ent->g.touch = 0;
				door_go_down(ent);
			}
			return;
		}
	}

#ifdef GROUND_ZERO
	// smart water is different
	if ((self.spawnflags & 2) && (gi.pointcontents((self.mins + self.maxs) * 0.5) & MASK_WATER))
	{
		self.message = 0;
		self.touch = 0;
		self.enemy = cactivator;
		smart_water_go_up (self);
		return;
	}
#endif

	// trigger all paired doors
	for (entityref ent = self; ent.has_value(); ent = ent->g.teamchain)
	{
		ent->g.message = nullptr;
		ent->g.touch = 0;
		door_go_up(ent, cactivator);
	}
}

static void Touch_DoorTrigger(entity &self, entity &other, vector, const surface &)
{
	if (other.g.health <= 0)
		return;

	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
		return;

	if ((self.owner->g.spawnflags & DOOR_NOMONSTER) && (other.svflags & SVF_MONSTER))
		return;

	if (level.framenum < self.g.touch_debounce_framenum)
		return;

	self.g.touch_debounce_framenum = level.framenum + (gtime)(1.0f * BASE_FRAMERATE);
	door_use(self.owner, other, other);
}

static void Think_CalcMoveSpeed(entity &self)
{
	if (self.g.flags & FL_TEAMSLAVE)
		return;     // only the team master does this

	// find the smallest distance any member of the team will be moving
	float min = fabs(self.g.moveinfo.distance);
	for (entityref ent = self.g.teamchain; ent.has_value(); ent = ent->g.teamchain)
	{
		float dist = fabs(ent->g.moveinfo.distance);
		if (dist < min)
			min = dist;
	}

	float time = min / self.g.moveinfo.speed;

	// adjust speeds so they will all complete at the same time
	for (entityref ent = self; ent.has_value(); ent = ent->g.teamchain)
	{
		float newspeed = fabs(ent->g.moveinfo.distance) / time;
		float ratio = newspeed / ent->g.moveinfo.speed;
		if (ent->g.moveinfo.accel == ent->g.moveinfo.speed)
			ent->g.moveinfo.accel = newspeed;
		else
			ent->g.moveinfo.accel *= ratio;
		if (ent->g.moveinfo.decel == ent->g.moveinfo.speed)
			ent->g.moveinfo.decel = newspeed;
		else
			ent->g.moveinfo.decel *= ratio;
		ent->g.moveinfo.speed = newspeed;
	}
}

static void Think_SpawnDoorTrigger(entity &ent)
{
	if (ent.g.flags & FL_TEAMSLAVE)
		return;     // only the team leader spawns a trigger

	vector cmins = ent.absmin;
	vector cmaxs = ent.absmax;

	for (entityref other = ent.g.teamchain; other.has_value(); other = other->g.teamchain)
	{
		AddPointToBounds(other->absmin, cmins, cmaxs);
		AddPointToBounds(other->absmax, cmins, cmaxs);
	}

	// expand
	cmins.x -= 60;
	cmins.y -= 60;
	cmaxs.x += 60;
	cmaxs.y += 60;

	entity &other = G_Spawn();
	other.mins = cmins;
	other.maxs = cmaxs;
	other.owner = ent;
	other.solid = SOLID_TRIGGER;
	other.g.movetype = MOVETYPE_NONE;
	other.g.touch = Touch_DoorTrigger;
	gi.linkentity(other);

	if (ent.g.spawnflags & DOOR_START_OPEN)
		door_use_areaportals(ent, true);

	Think_CalcMoveSpeed(ent);
}

static void door_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);

	if (self.g.spawnflags & DOOR_CRUSHER)
		return;

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self.g.moveinfo.wait >= 0)
	{
		if (self.g.moveinfo.state == STATE_DOWN)
			for (entityref ent = self.g.teammaster; ent.has_value(); ent = ent->g.teamchain)
				door_go_up(ent, ent->g.activator);
		else
			for (entityref ent = self.g.teammaster ; ent.has_value(); ent = ent->g.teamchain)
				door_go_down(ent);
	}
}

static void door_killed(entity &self, entity &, entity &attacker, int32_t, vector)
{
	for (entityref ent = self.g.teammaster; ent.has_value(); ent = ent->g.teamchain)
	{
		ent->g.health = ent->g.max_health;
		ent->g.takedamage = false;
	}

	door_use(self.g.teammaster, attacker, attacker);
}

static void door_touch(entity &self, entity &other, vector, const surface &)
{
	if (!other.is_client())
		return;

	if (level.framenum < self.g.touch_debounce_framenum)
		return;
	self.g.touch_debounce_framenum = level.framenum + (gtime)(5.0f * BASE_FRAMERATE);

	gi.centerprintf(other, "%s", self.g.message.ptr());
	gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
}

static void SP_func_door(entity &ent)
{
	if (ent.g.sounds != 1)
	{
		ent.g.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		ent.g.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		ent.g.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	G_SetMovedir(ent.s.angles, ent.g.movedir);
	ent.g.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.g.model);

	ent.g.blocked = door_blocked;
	ent.g.use = door_use;

	if (!ent.g.speed)
		ent.g.speed = 100.f;
#ifdef SINGLE_PLAYER
	if (deathmatch.intVal)
#endif
		ent.g.speed *= 2;

	if (!ent.g.accel)
		ent.g.accel = ent.g.speed;
	if (!ent.g.decel)
		ent.g.decel = ent.g.speed;

	if (!ent.g.wait)
		ent.g.wait = 3.f;
	if (!st.lip)
		st.lip = 8;
	if (!ent.g.dmg)
		ent.g.dmg = 2;

	// calculate second position
	ent.g.pos1 = ent.s.origin;
	vector abs_movedir;
	abs_movedir.x = fabs(ent.g.movedir.x);
	abs_movedir.y = fabs(ent.g.movedir.y);
	abs_movedir.z = fabs(ent.g.movedir.z);
	ent.g.moveinfo.distance = abs_movedir.x * ent.size.x + abs_movedir.y * ent.size.y + abs_movedir.z * ent.size.z - st.lip;
	ent.g.pos2 = ent.g.pos1 + (ent.g.moveinfo.distance * ent.g.movedir);

	// if it starts open, switch the positions
	if (ent.g.spawnflags & DOOR_START_OPEN)
	{
		ent.s.origin = ent.g.pos2;
		ent.g.pos2 = ent.g.pos1;
		ent.g.pos1 = ent.s.origin;
	}

	ent.g.moveinfo.state = STATE_BOTTOM;

	if (ent.g.health)
	{
		ent.g.takedamage = true;
		ent.g.die = door_killed;
		ent.g.max_health = ent.g.health;
	}
	else if (ent.g.targetname && ent.g.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.g.touch = door_touch;
	}

	ent.g.moveinfo.speed = ent.g.speed;
	ent.g.moveinfo.accel = ent.g.accel;
	ent.g.moveinfo.decel = ent.g.decel;
	ent.g.moveinfo.wait = ent.g.wait;
	ent.g.moveinfo.start_origin = ent.g.pos1;
	ent.g.moveinfo.start_angles = ent.s.angles;
	ent.g.moveinfo.end_origin = ent.g.pos2;
	ent.g.moveinfo.end_angles = ent.s.angles;

	if (ent.g.spawnflags & DOOR_ANIMATED)
		ent.s.effects |= EF_ANIM_ALL;
	if (ent.g.spawnflags & DOOR_ANIMATED_FAST)
		ent.s.effects |= EF_ANIM_ALLFAST;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent.g.team)
		ent.g.teammaster = ent;

	gi.linkentity(ent);

	ent.g.nextthink = level.framenum + 1;
	if (ent.g.health || ent.g.targetname)
		ent.g.think = Think_CalcMoveSpeed;
	else
		ent.g.think = Think_SpawnDoorTrigger;
}

REGISTER_ENTITY(func_door, ET_FUNC_DOOR);

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
static void(entity self, entity other, entity cactivator) Door_Activate =
{
	self.use = 0;
	
	if (self.health)
	{
		self.takedamage = true;
		self.die = door_killed;
		self.max_health = self.health;
	}

	if (self.health)
		self.think = Think_CalcMoveSpeed;
	else
		self.think = Think_SpawnDoorTrigger;
	self.nextthink = level.framenum + 1;

}
#endif

static void SP_func_door_rotating(entity &ent)
{
	ent.s.angles = vec3_origin;

	// set the axis of rotation
	ent.g.movedir = vec3_origin;
	if (ent.g.spawnflags & DOOR_X_AXIS)
		ent.g.movedir.z = 1.0f;
	else if (ent.g.spawnflags & DOOR_Y_AXIS)
		ent.g.movedir.x = 1.0f;
	else // Z_AXIS
		ent.g.movedir.y = 1.0f;

	// check for reverse rotation
	if (ent.g.spawnflags & DOOR_REVERSE)
		ent.g.movedir = -ent.g.movedir;

	if (!st.distance)
	{
		gi.dprintf("%s at %s with no distance set\n", ent.g.type, vtos(ent.s.origin).ptr());
		st.distance = 90;
	}

	ent.g.pos1 = ent.s.angles;
	ent.g.pos2 = ent.s.angles + (st.distance * ent.g.movedir);
	ent.g.moveinfo.distance = (float)st.distance;

	ent.g.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.g.model);

	ent.g.blocked = door_blocked;
	ent.g.use = door_use;

	if (!ent.g.speed)
		ent.g.speed = 100.f;
	if (!ent.g.accel)
		ent.g.accel = ent.g.speed;
	if (!ent.g.decel)
		ent.g.decel = ent.g.speed;

	if (!ent.g.wait)
		ent.g.wait = 3.f;
	if (!ent.g.dmg)
		ent.g.dmg = 2;

	if (ent.g.sounds != 1)
	{
		ent.g.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		ent.g.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		ent.g.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	// if it starts open, switch the positions
	if (ent.g.spawnflags & DOOR_START_OPEN)
	{
		ent.s.angles = ent.g.pos2;
		ent.g.pos2 = ent.g.pos1;
		ent.g.pos1 = ent.s.angles;
		ent.g.movedir = -ent.g.movedir;
	}

	if (ent.g.health)
	{
		ent.g.takedamage = true;
		ent.g.die = door_killed;
		ent.g.max_health = ent.g.health;
	}

	if (ent.g.targetname && ent.g.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.g.touch = door_touch;
	}

	ent.g.moveinfo.state = STATE_BOTTOM;
	ent.g.moveinfo.speed = ent.g.speed;
	ent.g.moveinfo.accel = ent.g.accel;
	ent.g.moveinfo.decel = ent.g.decel;
	ent.g.moveinfo.wait = ent.g.wait;
	ent.g.moveinfo.start_origin = ent.s.origin;
	ent.g.moveinfo.start_angles = ent.g.pos1;
	ent.g.moveinfo.end_origin = ent.s.origin;
	ent.g.moveinfo.end_angles = ent.g.pos2;

	if (ent.g.spawnflags & DOOR_ROTATING_ANIMATED)
		ent.s.effects |= EF_ANIM_ALL;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent.g.team)
		ent.g.teammaster = ent;

	gi.linkentity(ent);

	ent.g.nextthink = level.framenum + 1;
	if (ent.g.health || ent.g.targetname)
		ent.g.think = Think_CalcMoveSpeed;
	else
		ent.g.think = Think_SpawnDoorTrigger;

#ifdef GROUND_ZERO
	if (ent.spawnflags & DOOR_INACTIVE)
	{
		ent.takedamage = false;
		ent.die = 0;
		ent.think = 0;
		ent.nextthink = 0;
		ent.use = Door_Activate;
	}
#endif
}

REGISTER_ENTITY(func_door_rotating, ET_FUNC_DOOR_ROTATING);

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
static void(entity self, entity other) smart_water_blocked =
{
	if (!(other.svflags & SVF_MONSTER) && (!other.is_client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other.s.origin, vec3_origin, 100000, 1, 0, MOD_LAVA);
		// if it's still there, nuke it
		if (other.inuse)		// PGM
			BecomeExplosion1 (other);
		return;
	}

	T_Damage (other, self, self, vec3_origin, other.s.origin, vec3_origin, 100, 1, 0, MOD_LAVA);
}
#endif

static void SP_func_water(entity &self)
{
	G_SetMovedir(self.s.angles, self.g.movedir);
	self.g.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	gi.setmodel(self, self.g.model);

	switch (self.g.sounds)
	{
	case 1: // water
		self.g.moveinfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self.g.moveinfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;

	case 2: // lava
		self.g.moveinfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self.g.moveinfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;
	}

	// calculate second position
	self.g.pos1 = self.s.origin;
	vector	abs_movedir;
	abs_movedir.x = fabs(self.g.movedir.x);
	abs_movedir.y = fabs(self.g.movedir.y);
	abs_movedir.z = fabs(self.g.movedir.z);
	self.g.moveinfo.distance = abs_movedir.x * self.size.x + abs_movedir.y * self.size.y + abs_movedir.z * self.size.z - st.lip;
	self.g.pos2 = self.g.pos1 + (self.g.moveinfo.distance * self.g.movedir);

	// if it starts open, switch the positions
	if (self.g.spawnflags & DOOR_START_OPEN)
	{
		self.s.origin = self.g.pos2;
		self.g.pos2 = self.g.pos1;
		self.g.pos1 = self.s.origin;
	}

	self.g.moveinfo.start_origin = self.g.pos1;
	self.g.moveinfo.start_angles = self.s.angles;
	self.g.moveinfo.end_origin = self.g.pos2;
	self.g.moveinfo.end_angles = self.s.angles;

	self.g.moveinfo.state = STATE_BOTTOM;

	if (!self.g.speed)
		self.g.speed = 25.f;
	self.g.moveinfo.accel = self.g.moveinfo.decel = self.g.moveinfo.speed = self.g.speed;

#ifdef GROUND_ZERO
	if (self.spawnflags & 2)	// smart water
	{
		// this is actually the divisor of the lowest player's distance to determine speed.
		// self.speed then becomes the cap of the speed.
		if(!self.accel)
			self.accel = 20f;
		self.blocked = smart_water_blocked;
	}
#endif

	if (!self.g.wait)
		self.g.wait = -1.f;
	self.g.moveinfo.wait = self.g.wait;

	self.g.use = door_use;

	if (self.g.wait == -1)
		self.g.spawnflags |= DOOR_TOGGLE;

	gi.linkentity(self);
}

REGISTER_ENTITY(func_water, ET_FUNC_DOOR);

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed   default 100
dmg     default 2
noise   looping sound to play when the train is in motion

*/
constexpr spawn_flag TRAIN_START_ON		= (spawn_flag)1;
constexpr spawn_flag TRAIN_TOGGLE		= (spawn_flag)2;
constexpr spawn_flag TRAIN_BLOCK_STOPS	= (spawn_flag)4;

static void train_next(entity &self);

static void train_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	if (level.framenum < self.g.touch_debounce_framenum)
		return;

	if (!self.g.dmg)
		return;
	self.g.touch_debounce_framenum = level.framenum + (gtime)(0.5f * BASE_FRAMERATE);
	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

static void train_wait(entity &self)
{
	if (self.g.target_ent->g.pathtarget)
	{
		entity &ent = self.g.target_ent;
		string savetarget = ent.g.target;
		ent.g.target = ent.g.pathtarget;
		G_UseTargets(ent, self.g.activator);
		ent.g.target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self.inuse)
			return;
	}

	if (self.g.moveinfo.wait)
	{
		if (self.g.moveinfo.wait > 0)
		{
			self.g.nextthink = level.framenum + (gtime)(self.g.moveinfo.wait * BASE_FRAMERATE);
			self.g.think = train_next;
		}
		else if (self.g.spawnflags & TRAIN_TOGGLE)
		{ // && wait < 0
#ifdef GROUND_ZERO
			self.target_ent = 0;
#else
			train_next(self);
#endif
			self.g.spawnflags &= ~TRAIN_START_ON;
			self.g.velocity = vec3_origin;
			self.g.nextthink = 0;
		}

		if (!(self.g.flags & FL_TEAMSLAVE))
		{
			if (self.g.moveinfo.sound_end)
				gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_end, 1, ATTN_STATIC, 0);
			self.s.sound = SOUND_NONE;
		}
	}
	else
		train_next(self);
}

#ifdef GROUND_ZERO
static cvarref g_legacy_trains;

static void(entity self) train_piece_wait =
{
}
#endif

static void train_next(entity &self)
{
	bool first = true;

again:
	if (!self.g.target)
		return;

	entityref ent = G_PickTarget(self.g.target);
	if (!ent.has_value())
	{
		gi.dprintf("train_next: bad target %s\n", self.g.target.ptr());
		return;
	}

	self.g.target = ent->g.target;

	// check for a teleport path_corner
	if (ent->g.spawnflags & 1)
	{
		if (!first)
		{
			gi.dprintf("connected teleport path_corners, see %i at %s\n", ent->g.type, vtos(ent->s.origin).ptr());
			return;
		}
		first = false;
		self.s.origin = ent->s.origin - self.mins;
		self.s.old_origin = self.s.origin;
		self.s.event = EV_OTHER_TELEPORT;
		gi.linkentity(self);
		goto again;
	}

#ifdef GROUND_ZERO
	if (ent.speed)
	{
		self.speed = ent.speed;
		self.moveinfo.speed = ent.speed;
		if(ent.accel)
			self.moveinfo.accel = ent.accel;
		else
			self.moveinfo.accel = ent.speed;
		if(ent.decel)
			self.moveinfo.decel = ent.decel;
		else
			self.moveinfo.decel = ent.speed;
		self.moveinfo.current_speed = 0;
	}
#endif

	self.g.moveinfo.wait = ent->g.wait;
	self.g.target_ent = ent;

	if (!(self.g.flags & FL_TEAMSLAVE))
	{
		if (self.g.moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.g.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self.s.sound = self.g.moveinfo.sound_middle;
	}

	vector dest = ent->s.origin - self.mins;
	self.g.moveinfo.state = STATE_TOP;
	self.g.moveinfo.start_origin = self.s.origin;
	self.g.moveinfo.end_origin = dest;
	Move_Calc(self, dest, train_wait);
	self.g.spawnflags |= TRAIN_START_ON;

#ifdef GROUND_ZERO
	if(self.team && !g_legacy_trains.intVal)
	{
		entity e;
		vector dir, dst;

		dir = dest - self.s.origin;
		for (e=self.teamchain; e ; e = e.teamchain)
		{
			dst = dir + e.s.origin;
			e.moveinfo.start_origin = e.s.origin;
			e.moveinfo.end_origin = dst;

			e.moveinfo.state = STATE_TOP;
			e.speed = self.speed;
			e.moveinfo.speed = self.moveinfo.speed;
			e.moveinfo.accel = self.moveinfo.accel;
			e.moveinfo.decel = self.moveinfo.decel;
			e.movetype = MOVETYPE_PUSH;
			Move_Calc (e, dst, train_piece_wait);
		}
	
	}
#endif
}

static void train_resume(entity &self)
{
	entity &ent = self.g.target_ent;
	vector dest = ent.s.origin - self.mins;
	self.g.moveinfo.state = STATE_TOP;
	self.g.moveinfo.start_origin = self.s.origin;
	self.g.moveinfo.end_origin = dest;
	Move_Calc(self, dest, train_wait);
	self.g.spawnflags |= TRAIN_START_ON;
}

void func_train_find(entity &self)
{
	if (!self.g.target)
	{
		gi.dprintf("train_find: no target\n");
		return;
	}

	entityref ent = G_PickTarget(self.g.target);

	if (!ent.has_value())
	{
		gi.dprintf("train_find: target %s not found\n", self.g.target.ptr());
		return;
	}

	self.g.target = ent->g.target;

	self.s.origin = ent->s.origin - self.mins;
	gi.linkentity(self);

	// if not triggered, start immediately
	if (!self.g.targetname)
		self.g.spawnflags |= TRAIN_START_ON;

	if (self.g.spawnflags & TRAIN_START_ON)
	{
		self.g.nextthink = level.framenum + 1;
		self.g.think = train_next;
		self.g.activator = self;
	}
}

void train_use(entity &self, entity &, entity &cactivator)
{
	self.g.activator = cactivator;

	if (self.g.spawnflags & TRAIN_START_ON)
	{
		if (!(self.g.spawnflags & TRAIN_TOGGLE))
			return;
		self.g.spawnflags &= ~TRAIN_START_ON;
		self.g.velocity = vec3_origin;
		self.g.nextthink = 0;
	}
	else if (self.g.target_ent.has_value())
		train_resume(self);
	else
		train_next(self);
}

static void SP_func_train(entity &self)
{
#ifdef GROUND_ZERO
	g_legacy_trains = gi.cvar("g_legacy_trains", "0", CVAR_LATCH);
#endif

	self.g.movetype = MOVETYPE_PUSH;

	self.s.angles = vec3_origin;
	self.g.blocked = train_blocked;
	if (self.g.spawnflags & TRAIN_BLOCK_STOPS)
		self.g.dmg = 0;
	else if (!self.g.dmg)
		self.g.dmg = 100;
	self.solid = SOLID_BSP;
	gi.setmodel(self, self.g.model);

	if (st.noise)
		self.g.moveinfo.sound_middle = gi.soundindex(st.noise);

	if (!self.g.speed)
		self.g.speed = 100.f;

	self.g.moveinfo.accel = self.g.moveinfo.decel = self.g.moveinfo.speed = self.g.speed;

	self.g.use = train_use;

	gi.linkentity(self);

	if (self.g.target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self.g.nextthink = level.framenum + 1;
		self.g.think = func_train_find;
	}
	else
		gi.dprintf("func_train without a target at %s\n", vtos(self.absmin).ptr());
}

REGISTER_ENTITY(func_train, ET_FUNC_TRAIN);

/*QUAKED trigger_elevator (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
*/
static void trigger_elevator_use(entity &self, entity &other, entity &)
{
	if (self.g.movetarget->g.nextthink)
		return;

	if (!other.g.pathtarget)
	{
		gi.dprintf("elevator used with no pathtarget\n");
		return;
	}

	entityref target = G_PickTarget(other.g.pathtarget);
	if (!target.has_value())
	{
		gi.dprintf("elevator used with bad pathtarget: %s\n", other.g.pathtarget.ptr());
		return;
	}

	self.g.movetarget->g.target_ent = target;
	train_resume(self.g.movetarget);
}

static void trigger_elevator_init(entity &self)
{
	if (!self.g.target)
	{
		gi.dprintf("trigger_elevator has no target\n");
		return;
	}

	self.g.movetarget = G_PickTarget(self.g.target);

	if (!self.g.movetarget.has_value())
	{
		gi.dprintf("trigger_elevator unable to find target %s\n", self.g.target.ptr());
		return;
	}

	if (self.g.movetarget->g.type != ET_FUNC_TRAIN)
	{
		gi.dprintf("trigger_elevator target %s is not a train\n", self.g.target.ptr());
		return;
	}

	self.g.use = trigger_elevator_use;
	self.svflags = SVF_NOCLIENT;

}

static void SP_trigger_elevator(entity &self)
{
	self.g.think = trigger_elevator_init;
	self.g.nextthink = level.framenum + 1;
}

REGISTER_ENTITY(trigger_elevator, ET_TRIGGER_ELEVATOR);

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
	G_UseTargets(self, self.g.activator);
	self.g.nextthink = level.framenum + (gtime)((self.g.wait + random(-self.g.rand, self.g.rand)) * BASE_FRAMERATE);
}

static void func_timer_use(entity &self, entity &, entity &cactivator)
{
	self.g.activator = cactivator;

	// if on, turn it off
	if (self.g.nextthink)
	{
		self.g.nextthink = 0;
		return;
	}

	// turn it on
	if (self.g.delay)
		self.g.nextthink = level.framenum + (gtime)(self.g.delay * BASE_FRAMERATE);
	else
		func_timer_think(self);
}

static void SP_func_timer(entity &self)
{
	if (!self.g.wait)
		self.g.wait = 1.0f;

	self.g.use = func_timer_use;
	self.g.think = func_timer_think;

	if (self.g.rand >= self.g.wait)
	{
		self.g.rand = self.g.wait - FRAMETIME;
		gi.dprintf("func_timer at %s has random >= wait\n", vtos(self.s.origin).ptr());
	}

	if (self.g.spawnflags & 1)
	{
		self.g.nextthink = level.framenum + (gtime)((1.0f + st.pausetime + self.g.delay + self.g.wait + random(-self.g.rand, self.g.rand)) * BASE_FRAMERATE);
		self.g.activator = self;
	}

	self.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(func_timer, ET_FUNC_TIMER);

/*QUAKED func_conveyor (0 .5 .8) ? START_ON TOGGLE
Conveyors are stationary brushes that move what's on them.
The brush should be have a surface with at least one current content enabled.
speed   default 100
*/
constexpr spawn_flag CONVEYOR_START_ON = (spawn_flag)1;
constexpr spawn_flag CONVEYOR_TOGGLE = (spawn_flag)2;

static void func_conveyor_use(entity &self, entity &, entity &)
{
	if (self.g.spawnflags & 1)
	{
		self.g.speed = 0;
		self.g.spawnflags &= ~CONVEYOR_START_ON;
	}
	else
	{
		self.g.speed = (float)self.g.count;
		self.g.spawnflags |= CONVEYOR_START_ON;
	}

	if (!(self.g.spawnflags & CONVEYOR_TOGGLE))
		self.g.count = 0;
}

static void SP_func_conveyor(entity &self)
{
	if (!self.g.speed)
		self.g.speed = 100.f;

	if (!(self.g.spawnflags & CONVEYOR_START_ON))
	{
		self.g.count = (int32_t)self.g.speed;
		self.g.speed = 0;
	}

	self.g.use = func_conveyor_use;

	gi.setmodel(self, self.g.model);
	self.solid = SOLID_BSP;
	gi.linkentity(self);
}

REGISTER_ENTITY(func_conveyor, ET_FUNC_CONVEYOR);

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
static void door_secret_move2(entity &self);
static void door_secret_move3(entity &self);
static void door_secret_move4(entity &self);
static void door_secret_move5(entity &self);
static void door_secret_move6(entity &self);
static void door_secret_done(entity &self);

static void door_secret_use(entity &self, entity &, entity &)
{
	// make sure we're not already moving
	if (self.s.origin)
		return;

	Move_Calc(self, self.g.pos1, door_secret_move1);
	door_use_areaportals(self, true);
}

static void door_secret_move1(entity &self)
{
	self.g.nextthink = level.framenum + (gtime)(1.0f * BASE_FRAMERATE);
	self.g.think = door_secret_move2;
}

static void door_secret_move2(entity &self)
{
	Move_Calc(self, self.g.pos2, door_secret_move3);
}

static void door_secret_move3(entity &self)
{
	if (self.g.wait == -1)
		return;
	self.g.nextthink = level.framenum + (gtime)(self.g.wait * BASE_FRAMERATE);
	self.g.think = door_secret_move4;
}

static void door_secret_move4(entity &self)
{
	Move_Calc(self, self.g.pos1, door_secret_move5);
}

static void door_secret_move5(entity &self)
{
	self.g.nextthink = level.framenum + (gtime)(1.0f * BASE_FRAMERATE);
	self.g.think = door_secret_move6;
}

static void door_secret_move6(entity &self)
{
	Move_Calc(self, vec3_origin, door_secret_done);
}

static void door_secret_done(entity &self)
{
	if (!self.g.targetname || (self.g.spawnflags & SECRET_ALWAYS_SHOOT))
	{
		self.g.health = 0;
		self.g.takedamage = true;
	}
	door_use_areaportals(self, false);
}

static void door_secret_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client())
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1(other);
		return;
	}

	if (level.framenum < self.g.touch_debounce_framenum)
		return;

	self.g.touch_debounce_framenum = level.framenum + (gtime)(0.5f * BASE_FRAMERATE);

	T_Damage(other, self, self, vec3_origin, other.s.origin, vec3_origin, self.g.dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

static void door_secret_die(entity &self, entity &, entity &attacker, int32_t, vector)
{
	self.g.takedamage = false;
	door_secret_use(self, attacker, attacker);
}

static void SP_func_door_secret(entity &ent)
{
	ent.g.moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
	ent.g.moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
	ent.g.moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");

	ent.g.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel(ent, ent.g.model);

	ent.g.blocked = door_secret_blocked;
	ent.g.use = door_secret_use;

	if (!ent.g.targetname || (ent.g.spawnflags & SECRET_ALWAYS_SHOOT))
	{
		ent.g.health = 0;
		ent.g.takedamage = true;
		ent.g.die = door_secret_die;
	}

	if (!ent.g.dmg)
		ent.g.dmg = 2;

	if (!ent.g.wait)
		ent.g.wait = 5.f;

	ent.g.moveinfo.accel =
		ent.g.moveinfo.decel =
			ent.g.moveinfo.speed = 50.f;

	// calculate positions
	vector forward, right, up;
	AngleVectors(ent.s.angles, &forward, &right, &up);
	ent.s.angles = vec3_origin;

	float side = 1.0f - (float)(ent.g.spawnflags & SECRET_1ST_LEFT);
	float width;

	if (ent.g.spawnflags & SECRET_1ST_DOWN)
		width = fabs(up * ent.size);
	else
		width = fabs(right * ent.size);

	float length = fabs(forward * ent.size);

	if (ent.g.spawnflags & SECRET_1ST_DOWN)
		ent.g.pos1 = ent.s.origin + ((-1 * width) * up);
	else
		ent.g.pos1 = ent.s.origin + ((side * width) * right);
	ent.g.pos2 = ent.g.pos1 + (length * forward);

	if (ent.g.health)
	{
		ent.g.takedamage = true;
		ent.g.die = door_killed;
		ent.g.max_health = ent.g.health;
	}
	else if (ent.g.targetname && ent.g.message)
	{
		gi.soundindex("misc/talk.wav");
		ent.g.touch = door_touch;
	}

	gi.linkentity(ent);
}

REGISTER_ENTITY(func_door_secret, ET_FUNC_DOOR);

/*QUAKED func_killbox (1 0 0) ?
Kills everything inside when fired, irrespective of protection.
*/
static void use_killbox(entity &self, entity &, entity &)
{
	KillBox(self);
}

static void SP_func_killbox(entity &ent)
{
	gi.setmodel(ent, ent.g.model);
	ent.g.use = use_killbox;
	ent.svflags = SVF_NOCLIENT;
}

REGISTER_ENTITY(func_killbox, ET_FUNC_KILLBOX);
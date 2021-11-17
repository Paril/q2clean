#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/func.h"
#include "game/game.h"
#include "game/spawn.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/combat.h"
#include "func.h"

#ifdef ROGUE_AI
#include "ai.h"
#endif

/*
=============================================================================

SECRET DOORS

=============================================================================
*/

constexpr spawn_flag SEC_OPEN_ONCE = (spawn_flag) 1;		// stay open
constexpr spawn_flag SEC_1ST_DOWN = (spawn_flag) 4;		// 1st move is down from arrow
constexpr spawn_flag SEC_YES_SHOOT = (spawn_flag) 16;		// shootable even if targeted
constexpr spawn_flag SEC_MOVE_RIGHT = (spawn_flag) 32;
constexpr spawn_flag SEC_MOVE_FORWARD = (spawn_flag) 64;

static void fd_secret_move1(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move1);

static void fd_secret_use(entity &self, entity &, entity &)
{
	// trigger all paired doors
	if (!(self.flags & FL_TEAMSLAVE))
		for (entity &ent : G_IterateChain<&entity::teamchain>(self))
			Move_Calc(ent, ent.moveinfo.start_origin, SAVABLE(fd_secret_move1));
}

REGISTER_STATIC_SAVABLE(fd_secret_use);

static void fd_secret_killed(entity &self, entity &inflictor, entity &attacker, int32_t damage, vector point)
{
	self.health = self.max_health;
	self.takedamage = false;

	if ((self.flags & FL_TEAMSLAVE) && self.teammaster.has_value() && self.teammaster->takedamage != false)
		fd_secret_killed(self.teammaster, inflictor, attacker, damage, point);
	else
		fd_secret_use(self, inflictor, attacker);
}

REGISTER_STATIC_SAVABLE(fd_secret_killed);

static void fd_secret_move2(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move2);

// Wait after first movement...
static void fd_secret_move1(entity &self)
{
	self.nextthink = level.time + 1s;
	self.think = SAVABLE(fd_secret_move2);
}

static void fd_secret_move3(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move3);

// Start moving sideways w/sound...
static void fd_secret_move2(entity &self)
{
	Move_Calc(self, self.moveinfo.end_origin, SAVABLE(fd_secret_move3));
}

static void fd_secret_move4(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move4);

// Wait here until time to go back...
static void fd_secret_move3(entity &self)
{
	if (!(self.spawnflags & SEC_OPEN_ONCE))
	{
		self.nextthink = duration_cast<gtime>(level.time + self.wait);
		self.think = SAVABLE(fd_secret_move4);
	}
}

static void fd_secret_move5(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move5);

// Move backward...
static void fd_secret_move4(entity &self)
{
	Move_Calc(self, self.moveinfo.start_origin, SAVABLE(fd_secret_move5));          
}

static void fd_secret_move6(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_move6);

// Wait 1 second...
static void fd_secret_move5(entity &self)
{
	self.nextthink = level.time + 1s;
	self.think = SAVABLE(fd_secret_move6);
}

static void fd_secret_done(entity &self);

REGISTER_STATIC_SAVABLE(fd_secret_done);

static void fd_secret_move6(entity &self)
{
	Move_Calc(self, self.move_origin, SAVABLE(fd_secret_done));
}

static void fd_secret_done(entity &self)
{
	if (!self.targetname || self.spawnflags & SEC_YES_SHOOT)
	{
		self.health = 1;
		self.takedamage = true;
		self.die = SAVABLE(fd_secret_killed);   
	}
}

static void secret_blocked(entity &self, entity &other)
{
	if (!(self.flags & FL_TEAMSLAVE))
		T_Damage (other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 0, { DAMAGE_NONE }, MOD_CRUSH);
}

REGISTER_STATIC_SAVABLE(secret_blocked);

/*
================
secret_touch

Prints messages
================
*/
static void secret_touch(entity &self, entity &other, vector, const surface &)
{
	if (other.health <= 0)
		return;

	if (!other.is_client)
		return;

	if (self.touch_debounce_time > level.time)
		return;

	self.touch_debounce_time = level.time + 2s;
	
	if (self.message)
		gi.centerprint(other, self.message);
}

REGISTER_STATIC_SAVABLE(secret_touch);

/*QUAKED func_door_secret2 (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot slide_right slide_forward
Basic secret door. Slides back, then to the left. Angle determines direction.

FLAGS:
open_once = not implemented yet
1st_left = 1st move is left/right of arrow
1st_down = 1st move is forwards/backwards
no_shoot = not implemented yet
always_shoot = even if targeted, keep shootable
reverse_left = the sideways move will be to right of arrow
reverse_back = the to/fro move will be forward

VALUES:
wait = # of seconds before coming back (5 default)
dmg  = damage to inflict when blocked (2 default)

*/
static void SP_func_door_secret2(entity &ent)
{
	ent.moveinfo.sound_start = gi.soundindex  ("doors/dr1_strt.wav");
	ent.moveinfo.sound_middle = gi.soundindex  ("doors/dr1_mid.wav");
	ent.moveinfo.sound_end = gi.soundindex  ("doors/dr1_end.wav");

	if (!ent.dmg)
		ent.dmg = 2;

	vector forward,right,up;
	AngleVectors(ent.angles, &forward, &right, &up);

	ent.move_origin = ent.origin;
	ent.move_angles = ent.angles;

	G_SetMovedir (ent.angles, ent.movedir);
	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	gi.setmodel (ent, ent.model);

	float lrSize, fbSize;

	if(ent.move_angles[1] == 0 || ent.move_angles[1] == 180)
	{
		lrSize = ent.size[1];
		fbSize = ent.size[0];
	}		
	else if(ent.move_angles[1] == 90 || ent.move_angles[1] == 270)
	{
		lrSize = ent.size[0];
		fbSize = ent.size[1];
	}
	else
	{
		gi.dprintfmt("{}: angle not at 0,90,180,270\n", ent);
		lrSize = fbSize = 0;
	}

	if (ent.spawnflags & SEC_MOVE_FORWARD)
		forward *= fbSize;
	else
		forward *= -fbSize;

	if(ent.spawnflags & SEC_MOVE_RIGHT)
		right *= lrSize;
	else
		right *= -lrSize;

	if (ent.spawnflags & SEC_1ST_DOWN)
	{
		ent.moveinfo.start_origin = ent.origin + forward;
		ent.moveinfo.end_origin = ent.moveinfo.start_origin + right;
	}
	else
	{
		ent.moveinfo.start_origin = ent.origin + right;
		ent.moveinfo.end_origin = ent.moveinfo.start_origin + forward;
	}

	ent.touch = SAVABLE(secret_touch);
	ent.blocked = SAVABLE(secret_blocked);
	ent.use = SAVABLE(fd_secret_use);
	ent.moveinfo.speed = 50.f;
	ent.moveinfo.accel = 50.f;
	ent.moveinfo.decel = 50.f;

	if (!ent.targetname || ent.spawnflags & SEC_YES_SHOOT)
	{
		ent.health = 1;
		ent.max_health = ent.health;
		ent.takedamage = true;
		ent.die = SAVABLE(fd_secret_killed);
	}
	if (ent.wait == gtime::zero())
		ent.wait = 5s;          // 5 seconds before closing

	gi.linkentity(ent);
}

static REGISTER_ENTITY_INHERIT(FUNC_DOOR_SECRET2, func_door_secret2, ET_FUNC_DOOR);

// ==================================================

constexpr spawn_flag FWALL_START_ON = (spawn_flag) 1;

static void force_wall_think(entity &self);

REGISTER_STATIC_SAVABLE(force_wall_think);

static void force_wall_think(entity &self)
{
	if (self.wait == gtime::zero())
		gi.ConstructMessage(svc_temp_entity, TE_FORCEWALL, self.pos1, self.pos2, (uint8_t) self.style).multicast(self.offset, MULTICAST_PVS);

	self.think = SAVABLE(force_wall_think);
	self.nextthink = level.time + 1_hz;
}

static void force_wall_use(entity &self, entity &, entity &)
{
	if (self.wait == gtime::zero())
	{
		self.wait = 1s;
		self.think = nullptr;
		self.nextthink = gtime::zero();
		self.solid = SOLID_NOT;
		gi.linkentity(self);
	}
	else
	{
		self.wait = gtime::zero();
		self.think = SAVABLE(force_wall_think);
		self.nextthink = level.time + 1_hz;
		self.solid = SOLID_BSP;
		KillBox(self);		// Is this appropriate?
		gi.linkentity (self);
	}
}

REGISTER_STATIC_SAVABLE(force_wall_use);

/*QUAKED func_force_wall (1 0 1) ? start_on
A vertical particle force wall. Turns on and solid when triggered.
If someone is in the force wall when it turns on, they're telefragged.

start_on - forcewall begins activated. triggering will turn it off.
style - color of particles to use.
	208: green, 240: red, 241: blue, 224: orange
*/
static void SP_func_force_wall(entity &ent)
{
	gi.setmodel (ent, ent.model);

	ent.offset = ent.absbounds.center();

	ent.pos1[2] = ent.absbounds.maxs[2];
	ent.pos2[2] = ent.absbounds.maxs[2];

	if (ent.size[0] > ent.size[1])
	{
		ent.pos1[0] = ent.absbounds.mins[0];
		ent.pos2[0] = ent.absbounds.maxs[0];
		ent.pos1[1] = ent.offset[1];
		ent.pos2[1] = ent.offset[1];
	}
	else
	{
		ent.pos1[0] = ent.offset[0];
		ent.pos2[0] = ent.offset[0];
		ent.pos1[1] = ent.absbounds.mins[1];
		ent.pos2[1] = ent.absbounds.maxs[1];
	}
	
	if (!ent.style)
		ent.style = 208;

	ent.movetype = MOVETYPE_NONE;
	ent.wait = 1s;

	if (ent.spawnflags & FWALL_START_ON)
	{
		ent.solid = SOLID_BSP;
		ent.think = SAVABLE(force_wall_think);
		ent.nextthink = level.time + 1_hz;
	}
	else
		ent.solid = SOLID_NOT;

	ent.use = SAVABLE(force_wall_use);
	ent.svflags = SVF_NOCLIENT;
	
	gi.linkentity(ent);
}

static REGISTER_ENTITY(FUNC_FORCE_WALL, func_force_wall);

constexpr spawn_flag PLAT2_TOGGLE			= (spawn_flag) 2;
constexpr spawn_flag PLAT2_TOP				= (spawn_flag) 4;
constexpr spawn_flag PLAT2_BOX_LIFT			= (spawn_flag) 32;

// ==========================================
// PLAT 2
// ==========================================

static void plat2_go_down(entity &ent);

REGISTER_STATIC_SAVABLE(plat2_go_down);

static void plat2_go_up(entity &ent);

REGISTER_STATIC_SAVABLE(plat2_go_up);

#ifdef ROGUE_AI
void plat2_spawn_danger_area(entity &ent)
{
	vector cmaxs = ent.bounds.maxs;
	cmaxs[2] = ent.bounds.mins[2] + 64.f;

	SpawnBadArea(ent.bounds.mins, cmaxs, gtime::zero(), ent);
}

void plat2_kill_danger_area(entity &ent)
{
	for (entity &t : G_IterateEquals<&entity::type>(ET_BAD_AREA))
		if (t.owner == ent)
			G_FreeEdict(t);
}
#endif

static void plat2_hit_top(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.moveinfo.sound_end, ATTN_STATIC);
		ent.sound = SOUND_NONE;
	}

	ent.moveinfo.state = STATE_TOP;

	if (ent.plat2flags & PLAT2_CALLED)
	{
		ent.plat2flags = PLAT2_WAITING;

		if (!(ent.spawnflags & PLAT2_TOGGLE))
		{
			ent.think = SAVABLE(plat2_go_down);
			ent.nextthink = level.time + 5s;
		}

#ifdef SINGLE_PLAYER
		if (!deathmatch)
			ent.last_move_time = level.time - 2s;
		else
#endif
			ent.last_move_time = level.time - 1s;
	}
	else if (!(ent.spawnflags & PLAT2_TOP) && !(ent.spawnflags & PLAT2_TOGGLE))
	{
		ent.plat2flags = PLAT2_NONE;
		ent.think = SAVABLE(plat2_go_down);
		ent.nextthink = level.time + 2s;
		ent.last_move_time = level.time;
	}
	else
	{
		ent.plat2flags = PLAT2_NONE;
		ent.last_move_time = level.time;
	}

	G_UseTargets (ent, ent);
}

REGISTER_STATIC_SAVABLE(plat2_hit_top);

static void plat2_hit_bottom(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_end)
			gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.moveinfo.sound_end, ATTN_STATIC);
		ent.sound = SOUND_NONE;
	}
	ent.moveinfo.state = STATE_BOTTOM;
	
	if (ent.plat2flags & PLAT2_CALLED)
	{
		ent.plat2flags = PLAT2_WAITING;

		if (!(ent.spawnflags & PLAT2_TOGGLE))
		{
			ent.think = SAVABLE(plat2_go_up);
			ent.nextthink = level.time + 5s;
		}
#ifdef SINGLE_PLAYER

		if (!deathmatch)
			ent.last_move_time = level.time - 2s;
		else
#endif
			ent.last_move_time = level.time - 1s;
	}
	else if ((ent.spawnflags & PLAT2_TOP) && !(ent.spawnflags & PLAT2_TOGGLE))
	{
		ent.plat2flags = PLAT2_NONE;
		ent.think = SAVABLE(plat2_go_up);
		ent.nextthink = level.time + 2s;
		ent.last_move_time = level.time;
	}
	else
	{
		ent.plat2flags = PLAT2_NONE;
		ent.last_move_time = level.time;
	}

#ifdef ROGUE_AI
	plat2_kill_danger_area (ent);
#endif
	G_UseTargets (ent, ent);
}

REGISTER_STATIC_SAVABLE(plat2_hit_bottom);

static void plat2_go_down(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.moveinfo.sound_start, ATTN_STATIC);
		ent.sound = ent.moveinfo.sound_middle;
	}
	ent.moveinfo.state = STATE_DOWN;
	ent.plat2flags |= PLAT2_MOVING;

	Move_Calc (ent, ent.moveinfo.end_origin, SAVABLE(plat2_hit_bottom));
}

static void plat2_go_up(entity &ent)
{
	if (!(ent.flags & FL_TEAMSLAVE))
	{
		if (ent.moveinfo.sound_start)
			gi.sound (ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent.moveinfo.sound_start, ATTN_STATIC);
		ent.sound = ent.moveinfo.sound_middle;
	}
	ent.moveinfo.state = STATE_UP;
	ent.plat2flags |= PLAT2_MOVING;

#ifdef ROGUE_AI
	plat2_spawn_danger_area(ent);
#endif
	Move_Calc (ent, ent.moveinfo.start_origin, SAVABLE(plat2_hit_top));
}

static void plat2_operate(entity &trigger, entity &other)
{
	entity &ent = trigger.enemy;

	if (ent.plat2flags & PLAT2_MOVING)
		return;

	if ((ent.last_move_time + 2s) > level.time)
		return;

	float platCenter = (trigger.absbounds.mins[2] + trigger.absbounds.maxs[2]) / 2;
	move_state otherState;

	if (ent.moveinfo.state == STATE_TOP)
	{
		if (ent.spawnflags & PLAT2_BOX_LIFT)
		{
			if (platCenter > other.origin[2])
				otherState = STATE_BOTTOM;
			else
				otherState = STATE_TOP;
		}
		else
		{
			if (trigger.absbounds.maxs[2] > other.origin[2])
				otherState = STATE_BOTTOM;
			else
				otherState = STATE_TOP;
		}
	}
	else if (other.origin[2] > platCenter)
		otherState = STATE_TOP;
	else
		otherState = STATE_BOTTOM;

	ent.plat2flags = PLAT2_MOVING;

#ifdef SINGLE_PLAYER
	gtime pauseTime;

	if (!deathmatch)
		pauseTime = 500ms;
	else
		pauseTime = 300ms;
#else
	gtime pauseTime = 300ms;
#endif

	if (ent.moveinfo.state != otherState)
	{
		ent.plat2flags |= PLAT2_CALLED;
		pauseTime = 100ms;
	}

	ent.last_move_time = level.time;
	
	if (ent.moveinfo.state == STATE_BOTTOM)
		ent.think = SAVABLE(plat2_go_up);
	else
		ent.think = SAVABLE(plat2_go_down);

	ent.nextthink = level.time + pauseTime;
}

static void Touch_Plat_Center2(entity &ent, entity &other, vector, const surface &)
{
	if (other.health <= 0)
		return;

	// PMM - don't let non-monsters activate plat2s
	if (!(other.svflags & SVF_MONSTER) && !other.is_client)
		return;
	
	plat2_operate(ent, other);
}

REGISTER_STATIC_SAVABLE(Touch_Plat_Center2);

static void plat2_blocked(entity &self, entity &other)
{
	if (!(other.svflags & SVF_MONSTER) && !other.is_client)
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other.origin, vec3_origin, 100000, 1, { DAMAGE_NONE }, MOD_CRUSH);

		// if it's still there, nuke it
		if (other.inuse)
			BecomeExplosion1 (other);
		return;
	}

	// gib dead things
	if (other.health < 1)
		T_Damage(other, self, self, vec3_origin, other.origin, vec3_origin, 100, 1, { DAMAGE_NONE }, MOD_CRUSH);

	T_Damage (other, self, self, vec3_origin, other.origin, vec3_origin, self.dmg, 1, { DAMAGE_NONE }, MOD_CRUSH);

	if (self.moveinfo.state == STATE_UP)
		plat2_go_down (self);
	else if (self.moveinfo.state == STATE_DOWN)
		plat2_go_up (self);
}

REGISTER_STATIC_SAVABLE(plat2_blocked);

static void Use_Plat2(entity &ent, entity &, entity &cactivator)
{ 
	if (ent.moveinfo.state > STATE_BOTTOM)
		return;
	if ((ent.last_move_time + 2s) > level.time)
		return;

	for (entity &trigger : entity_range(game.maxclients + 1, num_entities - 1))
	{
		if (!trigger.inuse)
			continue;

		if (trigger.touch == Touch_Plat_Center2 && trigger.enemy == ent)
		{
			plat2_operate (trigger, cactivator);
			return;
		}
	}
}

REGISTER_STATIC_SAVABLE(Use_Plat2);

static void plat2_activate(entity &ent, entity &, entity &)
{
	ent.use = SAVABLE(Use_Plat2);

	entity &trigger = plat_spawn_inside_trigger(ent);	// the "start moving" trigger	

	trigger.bounds.maxs[0] += 10;
	trigger.bounds.maxs[1] += 10;
	trigger.bounds.mins[0] -= 10;
	trigger.bounds.mins[1] -= 10;

	gi.linkentity (trigger);
	
	trigger.touch = SAVABLE(Touch_Plat_Center2);		// Override trigger touch function

	plat2_go_down(ent);
}

REGISTER_STATIC_SAVABLE(plat2_activate);

/*QUAKED func_plat2 (0 .5 .8) ? PLAT_LOW_TRIGGER PLAT2_TOGGLE PLAT2_TOP PLAT2_TRIGGER_TOP PLAT2_TRIGGER_BOTTOM BOX_LIFT
speed	default 150

PLAT_LOW_TRIGGER - creates a short trigger field at the bottom
PLAT2_TOGGLE - plat will not return to default position.
PLAT2_TOP - plat's default position will the the top.
PLAT2_TRIGGER_TOP - plat will trigger it's targets each time it hits top
PLAT2_TRIGGER_BOTTOM - plat will trigger it's targets each time it hits bottom
BOX_LIFT - this indicates that the lift is a box, rather than just a platform

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

"speed"	overrides default 200.
"accel" overrides default 500
"lip"	no default

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determoveinfoned by the model's height.

*/
static void SP_func_plat2(entity &ent)
{
	ent.angles = vec3_origin;
	ent.solid = SOLID_BSP;
	ent.movetype = MOVETYPE_PUSH;

	gi.setmodel (ent, ent.model);

	ent.blocked = SAVABLE(plat2_blocked);

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
#ifdef SINGLE_PLAYER

	if (deathmatch)
	{
#endif
		ent.speed *= 2;
		ent.accel *= 2;
		ent.decel *= 2;
#ifdef SINGLE_PLAYER
	}
#endif

	//PMM Added to kill things it's being blocked by 
	if (!ent.dmg)
		ent.dmg = 2;

	// pos1 is the top position, pos2 is the bottom
	ent.pos1 = ent.origin;
	ent.pos2 = ent.origin;

	if (st.height)
		ent.pos2[2] -= (st.height - st.lip);
	else
		ent.pos2[2] -= (ent.bounds.maxs[2] - ent.bounds.mins[2]) - st.lip;

	ent.moveinfo.state = STATE_TOP;

	if (ent.targetname)
		ent.use = SAVABLE(plat2_activate);
	else
	{
		ent.use = SAVABLE(Use_Plat2);

		entity &trigger = plat_spawn_inside_trigger(ent);	// the "start moving" trigger	

		trigger.bounds.maxs[0] += 10;
		trigger.bounds.maxs[1] += 10;
		trigger.bounds.mins[0] -= 10;
		trigger.bounds.mins[1] -= 10;

		gi.linkentity(trigger);

		trigger.touch = SAVABLE(Touch_Plat_Center2);		// Override trigger touch function

		if (!(ent.spawnflags & PLAT2_TOP))
		{
			ent.origin = ent.pos2;
			ent.moveinfo.state = STATE_BOTTOM;
		}	
	}

	gi.linkentity (ent);

	ent.moveinfo.speed = ent.speed;
	ent.moveinfo.accel = ent.accel;
	ent.moveinfo.decel = ent.decel;
	ent.moveinfo.wait = ent.wait;
	ent.moveinfo.start_origin = ent.pos1;
	ent.moveinfo.start_angles = ent.angles;
	ent.moveinfo.end_origin = ent.pos2;
	ent.moveinfo.end_angles = ent.angles;

	ent.moveinfo.sound_start = gi.soundindex ("plats/pt1_strt.wav");
	ent.moveinfo.sound_middle = gi.soundindex ("plats/pt1_mid.wav");
	ent.moveinfo.sound_end = gi.soundindex ("plats/pt1_end.wav");
}

static REGISTER_ENTITY_INHERIT(FUNC_PLAT2, func_plat2, ET_FUNC_PLAT);
#endif
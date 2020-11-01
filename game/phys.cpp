#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "../lib/set.h"
#include "game.h"
#include "phys.h"
#include "util.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

/*
============
SV_TestEntityPosition

============
*/
static inline bool SV_TestEntityPosition(entity &ent)
{
	content_flags	mask;

	if (ent.clipmask)
		mask = ent.clipmask;
	else
		mask = MASK_SOLID;

	return gi.trace(ent.s.origin, ent.mins, ent.maxs, ent.s.origin, ent, mask).startsolid;
}

/*
================
SV_CheckVelocity
bound velocity
================
*/
static inline void SV_CheckVelocity(entity &ent)
{
	vector normalized = ent.g.velocity;
	float length = VectorNormalize(normalized);

	if (length > (float)sv_maxvelocity)
		ent.g.velocity = normalized * (float)sv_maxvelocity;
}

/*
=============
SV_RunThink

Runs thinking code for this frame if necessary
=============
*/
static inline bool SV_RunThink(entity &ent)
{
	if (ent.g.nextthink <= 0 || ent.g.nextthink > level.framenum)
		return true;

	ent.g.nextthink = 0;

	if (!ent.g.think)
		gi.dprintf("NULL ent.think: %i @ %s", ent.g.type, vtos(ent.s.origin).ptr());
	else
		ent.g.think(ent);

	return false;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
static inline void SV_Impact(entity &e1, trace &tr)
{
	entity &e2 = tr.ent;

	if (e1.g.touch && e1.solid != SOLID_NOT)
		e1.g.touch(e1, e2, tr.normal, tr.surface);

	if (e2.g.touch && e2.solid != SOLID_NOT)
		e2.g.touch(e2, e1, vec3_origin, null_surface);
}

#ifdef SINGLE_PLAYER
/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
============
*/
static void SV_FlyMove(entity &ent, float time, content_flags mask)
{
	constexpr size_t numbumps = MAX_CLIP_PLANES - 1;

	array<vector, MAX_CLIP_PLANES>	planes;
	size_t numplanes = 0;
	vector new_velocity = vec3_origin;
	vector original_velocity = ent.g.velocity;
	vector primal_velocity = ent.g.velocity;
	float time_left = time;

	ent.g.groundentity = null_entity;

	for (size_t bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		vector end = ent.s.origin + time_left * ent.g.velocity;

		trace tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, end, ent, mask);

		if (tr.allsolid)
		{
			// entity is trapped in another solid
			ent.g.velocity = vec3_origin;
			return;
		}

		if (tr.fraction > 0)
		{
			// actually covered some distance
			ent.s.origin = tr.endpos;
			original_velocity = ent.g.velocity;
			numplanes = 0;
		}

		if (tr.fraction == 1)
			break;     // moved the entire distance

		entity &hit = tr.ent;

		if (tr.normal[2] > 0.7f)
		{
			if (hit.solid == SOLID_BSP)
			{
				ent.g.groundentity = hit;
				ent.g.groundentity_linkcount = hit.linkcount;
			}
		}

//
// run the impact function
//
		SV_Impact(ent, tr);

		if (!ent.inuse)
			break;      // removed by the impact function

		time_left -= time_left * tr.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			ent.g.velocity = vec3_origin;
			return;
		}

		planes[numplanes++] = tr.normal;

//
// modify original_velocity so it parallels all of the clip planes
//
		size_t i;
		for (i = 0; i < numplanes; i++)
		{
			new_velocity = ClipVelocity(original_velocity, planes[i], 1);

			size_t j;

			for (j = 0 ; j < numplanes ; j++)
				if (j != i && planes[i] != planes[j])
					if (new_velocity * planes[j] < 0)
						break;  // not ok

			if (j == numplanes)
				break;
		}

		if (i != numplanes)
			// go along this plane
			ent.g.velocity = new_velocity;
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
				ent.g.velocity = vec3_origin;
				return;
			}
			vector dir = CrossProduct(planes[0], planes[1]);
			float d = dir * ent.g.velocity;
			ent.g.velocity = dir * d;
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (ent.g.velocity * primal_velocity <= 0)
		{
			ent.g.velocity = vec3_origin;
			return;
		}
	}
}

#endif
/*
============
SV_AddGravity

============
*/
void SV_AddGravity(entity &ent)
{
#ifdef GROUND_ZERO
	if (ent.gravityVector[2] > 0)
		ent.velocity += ent.gravityVector * (ent.gravity * sv_gravity.floatVal * FRAMETIME);
	else
#endif
		ent.g.velocity.z -= ent.g.gravity * (float)sv_gravity * FRAMETIME;
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
static void SV_PushEntity(entity &ent, vector push, trace &tr)
{
	vector start = ent.s.origin;
	vector end = start + push;

retry:
	content_flags mask;

	if (ent.clipmask)
		mask = ent.clipmask;
	else
		mask = MASK_SOLID;

	tr = gi.trace(start, ent.mins, ent.maxs, end, ent, mask);
	
	ent.s.origin = tr.endpos;

	gi.linkentity(ent);

	if (tr.fraction != 1.0f)
	{
		SV_Impact(ent, tr);

		// if the pushed entity went away and the pusher is still there
		if (!tr.ent.inuse && ent.inuse)
		{
			// move the pusher back and try again
			ent.s.origin = start;
			gi.linkentity(ent);
			goto retry;
		}
	}

#ifdef GROUND_ZERO
	ent.gravity = 1.0;
#endif
	
	if (ent.inuse)
		G_TouchTriggers(ent);
}

static entityref			obstacle;
static dynarray<entityref>	pushed_list;

/*
============
SV_Push

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
============
*/
static bool SV_Push(entity &pusher, vector move, vector amove)
{
	// clamp the move to 1/8 units, so the position will
	// be accurate for client side prediction
	for (auto &v : move)
	{
		float temp = v * 8.0f;
		if (temp > 0.0f)
			temp += 0.5f;
		else
			temp -= 0.5f;
		v = 0.125f * (int32_t)temp;
	}
	
	// find the bounding box
	vector mins = pusher.absmin + move;
	vector maxs = pusher.absmax + move;

// we need this for pushing things later
	vector org = -amove, forward, right, up;
	AngleVectors(org, &forward, &right, &up);

// save the pusher's original position
	pushed_list.push_back(pusher);

	pusher.g.pushed.origin = pusher.s.origin;
	pusher.g.pushed.angles = pusher.s.angles;

// move the pusher to it's final position
	pusher.s.origin += move;
	pusher.s.angles += amove;
	gi.linkentity(pusher);

// see if any solid entities are inside the final position
	for (uint32_t e = 1; e < num_entities; e++)
	{
		entity &check = itoe(e);
		if (!check.inuse)
			continue;
		if (check.g.movetype == MOVETYPE_PUSH
			|| check.g.movetype == MOVETYPE_STOP
			|| check.g.movetype == MOVETYPE_NONE
			|| check.g.movetype == MOVETYPE_NOCLIP)
			continue;

		if (!check.is_linked())
			continue;       // not linked in anywhere

		// if the entity is standing on the pusher, it will definitely be moved
		if (check.g.groundentity != pusher)
		{
			// see if the ent needs to be tested
			if (check.absmin.x >= maxs.x
				|| check.absmin.y >= maxs.y
				|| check.absmin.z >= maxs.z
				|| check.absmax.x <= mins.x
				|| check.absmax.y <= mins.y
				|| check.absmax.z <= mins.z)
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}


		if ((pusher.g.movetype == MOVETYPE_PUSH) || (check.g.groundentity == pusher)) {
			// move this entity
			check.g.pushed.origin = check.s.origin;
			check.g.pushed.angles = check.s.angles;

			pushed_list.push_back(check);

			// try moving the contacted entity
			check.s.origin += move;

			// figure movement due to the pusher's amove
			org = check.s.origin - pusher.s.origin;
			vector org2;
			org2.x = org * forward;
			org2.y = -(org * right);
			org2.z = org * up;
			vector move2 = org2 - org;
			check.s.origin += move2;

			// may have pushed them off an edge
			if (check.g.groundentity != pusher)
				check.g.groundentity = null_entity;

			bool block = SV_TestEntityPosition(check);
			if (!block)
			{
				// pushed ok
				gi.linkentity(check);
				// impact?
				continue;
			}

			// if it is ok to leave in the old position, do it
			// this is only relevent for riding entities, not pushed
			// FIXME: this doesn't acount for rotation
			check.s.origin = check.s.origin - move;
			block = SV_TestEntityPosition(check);
			if (!block)
			{
				pushed_list.pop_back();
				continue;
			}
		}

		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for (auto it = pushed_list.rbegin(); it != pushed_list.rend(); it++)
		{
			entity &p = *it;
			p.s.origin = p.g.pushed.origin;
			p.s.angles = p.g.pushed.angles;
			gi.linkentity(p);
		}

		return false;
	}

	//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for (auto it = pushed_list.rbegin(); it != pushed_list.rend(); it++)
		G_TouchTriggers(*it);

	return true;
}

/*
================
SV_Physics_Pusher

Bmodel objects don't interact with each other, but
push all box objects
================
*/
static void SV_Physics_Pusher(entity &ent)
{
	// if not a team captain, so movement will be handled elsewhere
	if (ent.g.flags & FL_TEAMSLAVE)
		return;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_list.clear();

	entityref part = ent;

	for (; part.has_value(); part = part->g.teamchain)
	{
		if (part->g.velocity || part->g.avelocity)
		{
			// object is moving
			vector move = part->g.velocity * FRAMETIME;
			vector amove = part->g.avelocity * FRAMETIME;

			if (!SV_Push(part, move, amove))
				break;  // move was blocked
		}
	}

	if (part.has_value())
	{
		// the move failed, bump all nextthink times and back out moves
		for (entityref mv = ent; mv.has_value(); mv = mv->g.teamchain)
			if (mv->g.nextthink > 0)
				mv->g.nextthink++;

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (part->g.blocked)
			part->g.blocked(part, obstacle);
	}
	else
	{
		// the move succeeded, so call all think functions
		for (part = ent; part.has_value(); part = part->g.teamchain)
			if (part->inuse)
				SV_RunThink(part);
	}
}

//==================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
static inline void SV_Physics_None(entity &ent)
{
	// regular thinking
	SV_RunThink(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
static void SV_Physics_Noclip(entity &ent)
{
// regular thinking
	if (!SV_RunThink(ent))
		return;
	if (!ent.inuse)
		return;

	ent.s.angles += (FRAMETIME * ent.g.avelocity);
	ent.s.origin += (FRAMETIME * ent.g.velocity);

	gi.linkentity(ent);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
static void SV_Physics_Toss(entity &ent)
{
// regular thinking
	SV_RunThink(ent);
	if (!ent.inuse)
		return;

	// if not a team captain, so movement will be handled elsewhere
	if (ent.g.flags & FL_TEAMSLAVE)
		return;

	if (ent.g.velocity.z > 0)
		ent.g.groundentity = null_entity;
// check for the groundentity going away
	else if (ent.g.groundentity.has_value())
		if (!ent.g.groundentity->inuse)
			ent.g.groundentity = null_entity;

// if onground, return without moving
	if (ent.g.groundentity.has_value()
#ifdef GROUND_ZERO
		&& ent.gravity > 0
#endif
		)
		return;

	vector old_origin = ent.s.origin;

	SV_CheckVelocity(ent);

// add gravity
	if (ent.g.movetype != MOVETYPE_FLY
		&& ent.g.movetype != MOVETYPE_FLYMISSILE
#ifdef THE_RECKONING
		// RAFAEL
		// move type for rippergun projectile
		&& ent.movetype != MOVETYPE_WALLBOUNCE
#endif
		)
		SV_AddGravity(ent);

// move angles
	ent.s.angles += (FRAMETIME * ent.g.avelocity);

// move origin
	vector move = ent.g.velocity * FRAMETIME;
	trace tr;
	SV_PushEntity(ent, move, tr);
	if (!ent.inuse)
		return;

	if (tr.fraction < 1)
	{
		float backoff;

#ifdef THE_RECKONING
		// RAFAEL
		if (ent.movetype == MOVETYPE_WALLBOUNCE)
			backoff = 2.0;	
		else
#endif
		if (ent.g.movetype == MOVETYPE_BOUNCE)
			backoff = 1.5;
		else
			backoff = 1.f;

		ent.g.velocity = ClipVelocity(ent.g.velocity, tr.normal, backoff);

#ifdef THE_RECKONING
		// RAFAEL
		if (ent.movetype == MOVETYPE_WALLBOUNCE)
			ent.s.angles = vectoangles (ent.velocity);
#endif

		// stop if on ground
		if (tr.normal[2] > 0.7f
#ifdef THE_RECKONING
			 && ent.movetype != MOVETYPE_WALLBOUNCE
#endif
			)
		{
			if (ent.g.velocity.z < 60.f || ent.g.movetype != MOVETYPE_BOUNCE)
			{
				ent.g.groundentity = tr.ent;
				ent.g.groundentity_linkcount = tr.ent.linkcount;
				ent.g.velocity = vec3_origin;
				ent.g.avelocity = vec3_origin;
			}
		}
	}

// check for water transition
	bool wasinwater = (ent.g.watertype & MASK_WATER);
	ent.g.watertype = gi.pointcontents(ent.s.origin);
	bool isinwater = ent.g.watertype & MASK_WATER;

	if (isinwater)
		ent.g.waterlevel = 1;
	else
		ent.g.waterlevel = 0;

	if (!wasinwater && isinwater)
		gi.positioned_sound(old_origin, world, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);
	else if (wasinwater && !isinwater)
		gi.positioned_sound(ent.s.origin, world, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, ATTN_NORM, 0);

// move teamslaves
	for (entityref slave = ent.g.teamchain; slave.has_value(); slave = slave->g.teamchain)
	{
		slave->s.origin = ent.s.origin;
		gi.linkentity(slave);
	}
}

#ifdef SINGLE_PLAYER
/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/

//FIXME: hacked in for E3 demo
const float sv_stopspeed		= 100f;
const float sv_friction			= 6f;
const float sv_waterfriction	= 1f;

static void(entity ent) SV_AddRotationalFriction =
{
	ent.s.angles += (FRAMETIME * ent.avelocity);
	float adjustment = FRAMETIME * sv_stopspeed * sv_friction;

	for (int i = 0; i < 3; i++)
	{
		if (ent.avelocity[i] > 0)
		{
			ent.avelocity[i] -= adjustment;
			if (ent.avelocity[i] < 0)
				ent.avelocity[i] = 0;
		}
		else
		{
			ent.avelocity[i] += adjustment;
			if (ent.avelocity[i] > 0)
				ent.avelocity[i] = 0;
		}
	}
}

static void(entity ent) SV_Physics_Step =
{
	bool	wasonground;
	bool	hitsound = false;
	float	speed, newspeed, control;
	float	friction;
	entity	cgroundentity;
	int	mask;

	// airborn monsters should always check for ground
	if (ent.groundentity == null_entity)
		M_CheckGround(ent);

	cgroundentity = ent.groundentity;

	SV_CheckVelocity(ent);

	if (cgroundentity != null_entity)
		wasonground = true;
	else
		wasonground = false;

	if (ent.avelocity)
		SV_AddRotationalFriction(ent);

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	if (!wasonground)
		if (!(ent.flags & FL_FLY))
			if (!((ent.flags & FL_SWIM) && (ent.waterlevel > 2)))
			{
				if (ent.velocity.z < sv_gravity.floatVal * -0.1f)
					hitsound = true;
				if (ent.waterlevel == 0)
					SV_AddGravity(ent);
			}

	// friction for flying monsters that have been given vertical velocity
	if ((ent.flags & FL_FLY) && (ent.velocity.z != 0)) {
		speed = fabs(ent.velocity.z);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		friction = sv_friction / 3;
		newspeed = speed - (FRAMETIME * control * friction);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent.velocity.z *= newspeed;
	}

	// friction for flying monsters that have been given vertical velocity
	if ((ent.flags & FL_SWIM) && (ent.velocity.z != 0))
	{
		speed = fabs(ent.velocity.z);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		newspeed = speed - (FRAMETIME * control * sv_waterfriction * ent.waterlevel);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent.velocity.z *= newspeed;
	}

	if (ent.velocity)
	{
		// apply friction
		// let dead monsters who aren't completely onground slide
		if ((wasonground) || (ent.flags & (FL_SWIM | FL_FLY)))
			if (!(ent.health <= 0 && !M_CheckBottom(ent)))
			{
				speed = sqrt(ent.velocity.x * ent.velocity.x + ent.velocity.y * ent.velocity.y);
				if (speed) {
					friction = sv_friction;

					control = speed < sv_stopspeed ? sv_stopspeed : speed;
					newspeed = speed - FRAMETIME * control * friction;

					if (newspeed < 0)
						newspeed = 0;
					newspeed /= speed;

					ent.velocity.x *= newspeed;
					ent.velocity.y *= newspeed;
				}
			}

		if (ent.svflags & SVF_MONSTER)
			mask = MASK_MONSTERSOLID;
		else
			mask = MASK_SOLID;
		SV_FlyMove(ent, FRAMETIME, mask);

		gi.linkentity(ent);

#ifdef GROUND_ZERO
		ent.gravity = 1.0;
#endif
		
		G_TouchTriggers(ent);
		if (!ent.inuse)
			return;

		if (ent.groundentity != null_entity)
			if (!wasonground)
				if (hitsound)
					gi.sound(ent, 0, gi.soundindex("world/land.wav"), 1, 1, 0);
	}

// regular thinking
	if (ent.inuse)
		SV_RunThink(ent);
}
#endif

//============================================================================
/*
================
G_RunEntity

================
*/
void G_RunEntity(entity &ent)
{
	if (ent.g.prethink)
		ent.g.prethink(ent);

	switch (ent.g.movetype)
	{
	case MOVETYPE_PUSH:
	case MOVETYPE_STOP:
		SV_Physics_Pusher(ent);
		break;
	case MOVETYPE_NONE:
	case MOVETYPE_WALK:
		SV_Physics_None(ent);
		break;
	case MOVETYPE_NOCLIP:
		SV_Physics_Noclip(ent);
		break;
#ifdef SINGLE_PLAYER
	case MOVETYPE_STEP: {
#ifdef GROUND_ZERO
		trace_t	trace;
		vector previous_origin = ent.s.origin;
#endif
		SV_Physics_Step(ent);
#ifdef GROUND_ZERO
		// if we moved, check and fix origin if needed
		if (ent.s.origin != previous_origin)
		{
			gi.trace (&trace, ent.s.origin, ent.mins, ent.maxs, previous_origin, ent, MASK_MONSTERSOLID);
			if (trace.allsolid || trace.startsolid)
				ent.s.origin = previous_origin;
		}
#endif
		break; }
#endif
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
#ifdef THE_RECKONING
	// RAFAEL
	case MOVETYPE_WALLBOUNCE:
#endif
		SV_Physics_Toss(ent);
		break;
	default:
		gi.error("SV_Physics: bad movetype %i", ent.g.movetype);
	}
}

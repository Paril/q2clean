#include "config.h"
#include "entity.h"
#include "game.h"
#include "phys.h"
#include "lib/gi.h"
#include "game/util.h"
#include "lib/types/dynarray.h"
#include "lib/types/set.h"
#include "lib/string/format.h"

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

	return gi.trace(ent.origin, ent.bounds, ent.origin, ent, mask).startsolid;
}

/*
================
SV_CheckVelocity
bound velocity
================
*/
static inline void SV_CheckVelocity(entity &ent)
{
	vector normalized = ent.velocity;
	float length = VectorNormalize(normalized);

	if (length > sv_maxvelocity)
		ent.velocity = normalized * sv_maxvelocity.value;
}

/*
=============
SV_RunThink

Runs thinking code for this frame if necessary
=============
*/
static inline bool SV_RunThink(entity &ent)
{
	if (ent.nextthink <= 0 || ent.nextthink > level.framenum)
		return true;

	ent.nextthink = 0;

	if (!ent.think)
		gi.dprintfmt("{}: NULL think", ent);
	else
		ent.think(ent);

	return false;
}

void SV_Impact(entity &e1, const trace &tr)
{
	entity &e2 = tr.ent;

	if (e1.touch && e1.solid != SOLID_NOT)
		e1.touch(e1, e2, tr.normal, tr.surface);

	if (e2.touch && e2.solid != SOLID_NOT)
		e2.touch(e2, e1, vec3_origin, null_surface);
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
	vector original_velocity = ent.velocity;
	vector primal_velocity = ent.velocity;
	float time_left = time;

	ent.groundentity = null_entity;

	for (size_t bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		vector end = ent.origin + time_left * ent.velocity;

		trace tr = gi.trace(ent.origin, ent.bounds, end, ent, mask);

		if (tr.allsolid)
		{
			// entity is trapped in another solid
			ent.velocity = vec3_origin;
			return;
		}

		if (tr.fraction > 0)
		{
			// actually covered some distance
			ent.origin = tr.endpos;
			original_velocity = ent.velocity;
			numplanes = 0;
		}

		if (tr.fraction == 1)
			break;     // moved the entire distance

		entity &hit = tr.ent;

		if (tr.normal[2] > 0.7f)
		{
			if (hit.solid == SOLID_BSP)
			{
				ent.groundentity = hit;
				ent.groundentity_linkcount = hit.linkcount;
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
			ent.velocity = vec3_origin;
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
			ent.velocity = new_velocity;
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
				ent.velocity = vec3_origin;
				return;
			}
			vector dir = CrossProduct(planes[0], planes[1]);
			float d = dir * ent.velocity;
			ent.velocity = dir * d;
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (ent.velocity * primal_velocity <= 0)
		{
			ent.velocity = vec3_origin;
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
#ifdef ROGUE_AI
	if (ent.gravityVector[2] > 0)
		ent.velocity += ent.gravityVector * (ent.gravity * sv_gravity * FRAMETIME);
	else
#endif
		ent.velocity.z -= ent.gravity * sv_gravity * FRAMETIME;
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
	vector start = ent.origin;
	vector end = start + push;

retry:
	content_flags mask;

	if (ent.clipmask)
		mask = ent.clipmask;
	else
		mask = MASK_SOLID;

	tr = gi.trace(start, ent.bounds, end, ent, mask);
	
	ent.origin = tr.endpos;

	gi.linkentity(ent);

	if (tr.fraction != 1.0f)
	{
		SV_Impact(ent, tr);

		// if the pushed entity went away and the pusher is still there
		if (!tr.ent.inuse && ent.inuse)
		{
			// move the pusher back and try again
			ent.origin = start;
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
	vector mins = pusher.absbounds.mins + move;
	vector maxs = pusher.absbounds.maxs + move;

// we need this for pushing things later
	vector forward, right, up;
	AngleVectors(-amove, &forward, &right, &up);

// save the pusher's original position
	pushed_list.push_back(pusher);

	pusher.pushed.origin = pusher.origin;
	pusher.pushed.angles = pusher.angles;

// move the pusher to it's final position
	pusher.origin += move;
	pusher.angles += amove;
	gi.linkentity(pusher);

// see if any solid entities are inside the final position
	for (uint32_t e = 1; e < num_entities; e++)
	{
		entity &check = itoe(e);
		if (!check.inuse)
			continue;
		if (check.movetype == MOVETYPE_PUSH
			|| check.movetype == MOVETYPE_STOP
			|| check.movetype == MOVETYPE_NONE
			|| check.movetype == MOVETYPE_NOCLIP)
			continue;

		if (!check.is_linked())
			continue;       // not linked in anywhere

		// if the entity is standing on the pusher, it will definitely be moved
		if (check.groundentity != pusher)
		{
			// see if the ent needs to be tested
			if (check.absbounds.mins.x >= maxs.x
				|| check.absbounds.mins.y >= maxs.y
				|| check.absbounds.mins.z >= maxs.z
				|| check.absbounds.maxs.x <= mins.x
				|| check.absbounds.maxs.y <= mins.y
				|| check.absbounds.maxs.z <= mins.z)
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		if ((pusher.movetype == MOVETYPE_PUSH) || (check.groundentity == pusher)) {
			// move this entity
			check.pushed.origin = check.origin;
			check.pushed.angles = check.angles;

			pushed_list.push_back(check);

			// try moving the contacted entity
			check.origin += move;

			// figure movement due to the pusher's amove
			vector org = check.origin - pusher.origin;

			vector org2 = {
				org * forward,
				-(org * right),
				org * up
			};

			check.origin += org2 - org;

			// may have pushed them off an edge
			if (check.groundentity != pusher)
				check.groundentity = null_entity;

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
			check.origin = check.origin - move;
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
			p.origin = p.pushed.origin;
			p.angles = p.pushed.angles;
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
	if (ent.flags & FL_TEAMSLAVE)
		return;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_list.clear();

	entityref part = ent;

	for (; part.has_value(); part = part->teamchain)
	{
		if (part->velocity || part->avelocity)
		{
			// object is moving
			vector move = part->velocity * FRAMETIME;
			vector amove = part->avelocity * FRAMETIME;

			if (!SV_Push(part, move, amove))
				break;  // move was blocked
		}
	}

	if (part.has_value())
	{
		// the move failed, bump all nextthink times and back out moves
		for (entityref mv = ent; mv.has_value(); mv = mv->teamchain)
			if (mv->nextthink > 0)
				mv->nextthink++;

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (part->blocked)
			part->blocked(part, obstacle);
	}
	else
	{
		// the move succeeded, so call all think functions
		for (part = ent; part.has_value(); part = part->teamchain)
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

	ent.angles += (FRAMETIME * ent.avelocity);
	ent.origin += (FRAMETIME * ent.velocity);

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
	if (ent.flags & FL_TEAMSLAVE)
		return;

	if (ent.velocity.z > 0)
		ent.groundentity = null_entity;
// check for the groundentity going away
	else if (ent.groundentity.has_value())
		if (!ent.groundentity->inuse)
			ent.groundentity = null_entity;

// if onground, return without moving
	if (ent.groundentity.has_value()
#ifdef GROUND_ZERO
		&& ent.gravity > 0
#endif
		)
		return;

	vector old_origin = ent.origin;

	SV_CheckVelocity(ent);

// add gravity
	if (ent.movetype != MOVETYPE_FLY
		&& ent.movetype != MOVETYPE_FLYMISSILE
#ifdef THE_RECKONING
		// RAFAEL
		// move type for rippergun projectile
		&& ent.movetype != MOVETYPE_WALLBOUNCE
#endif
		)
		SV_AddGravity(ent);

// move angles
	ent.angles += (FRAMETIME * ent.avelocity);

// move origin
	vector move = ent.velocity * FRAMETIME;
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
		if (ent.movetype == MOVETYPE_BOUNCE)
			backoff = 1.5;
		else
			backoff = 1.f;

		ent.velocity = ClipVelocity(ent.velocity, tr.normal, backoff);

#ifdef THE_RECKONING
		// RAFAEL
		if (ent.movetype == MOVETYPE_WALLBOUNCE)
			ent.angles = vectoangles (ent.velocity);
#endif

		// stop if on ground
		if (tr.normal[2] > 0.7f
#ifdef THE_RECKONING
			 && ent.movetype != MOVETYPE_WALLBOUNCE
#endif
			)
		{
			if (ent.velocity.z < 60.f || ent.movetype != MOVETYPE_BOUNCE)
			{
				ent.groundentity = tr.ent;
				ent.groundentity_linkcount = tr.ent.linkcount;
				ent.velocity = ent.avelocity = vec3_origin;
			}
		}
	}

// check for water transition
	bool wasinwater = (ent.watertype & MASK_WATER);
	ent.watertype = gi.pointcontents(ent.origin);
	bool isinwater = ent.watertype & MASK_WATER;

	ent.waterlevel = isinwater ? WATER_LEGS : WATER_NONE;

	gi.positioned_sound(wasinwater ? ent.origin : old_origin, ent, gi.soundindex("misc/h2ohit1.wav"));

// move teamslaves
	for (entityref slave = ent.teamchain; slave.has_value(); slave = slave->teamchain)
	{
		slave->origin = ent.origin;
		gi.linkentity(slave);
	}
}

#ifdef SINGLE_PLAYER
/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

#include "monster.h"
#include "move.h"

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
const float sv_stopspeed		= 100.f;
const float sv_friction			= 6.f;
const float sv_waterfriction	= 1.f;

static void SV_AddRotationalFriction(entity &ent)
{
	ent.angles += (FRAMETIME * ent.avelocity);
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

static void SV_Physics_Step(entity &ent)
{
	bool	wasonground;
	bool	hitsound = false;
	float	speed, newspeed, control;
	float	friction;
	entityref cgroundentity;
	content_flags	mask;

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
	if (!wasonground &&
		!(ent.flags & FL_FLY) &&
		!((ent.flags & FL_SWIM) && (ent.waterlevel == WATER_UNDER)))
	{
		if (ent.velocity.z < sv_gravity * -0.1f)
			hitsound = true;
		if (!ent.waterlevel)
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
		newspeed = speed - (FRAMETIME * control * sv_waterfriction * (int32_t) ent.waterlevel);
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
					gi.sound(ent, CHAN_AUTO, gi.soundindex("world/land.wav"));
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
	if (ent.prethink)
		ent.prethink(ent);

	switch (ent.movetype)
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
		vector previous_origin = ent.origin;
#endif
		SV_Physics_Step(ent);
#ifdef GROUND_ZERO
		// if we moved, check and fix origin if needed
		if (ent.origin != previous_origin)
		{
			trace tr = gi.trace (ent.origin, ent.bounds, previous_origin, ent, MASK_MONSTERSOLID);

			if (tr.allsolid || tr.startsolid)
				ent.origin = previous_origin;
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
		gi.errorfmt("SV_Physics: bad movetype {}", (int32_t) ent.movetype);
	}
}

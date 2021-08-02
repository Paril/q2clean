#include "config.h"
#include "entity.h"
#include "move.h"
#include "phys.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"

#ifdef SINGLE_PLAYER

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#include "game/ai.h"
static entityref	new_bad;

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"
#endif
#ifdef THE_RECKONING
#include "game/xatrix/monster/fixbot.h"
#endif
#endif

bool M_CheckBottom(entity &ent)
{
	vector	cmins, cmaxs, start, stop;
	trace	trace;
	int	x, y;
	float	mid, bottom;

	cmins = ent.s.origin + ent.mins;
	cmaxs = ent.s.origin + ent.maxs;

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
#ifdef ROGUE_AI
	if(ent.gravityVector[2] > 0)
		start[2] = cmaxs[2] + 1;
	else
#endif
		start[2] = cmins[2] - 1;

	for (x = 0 ; x <= 1 ; x++)
		for (y = 0 ; y <= 1 ; y++) {
			start.x = x ? cmaxs.x : cmins.x;
			start.y = y ? cmaxs.y : cmins.y;
			if (gi.pointcontents(start) != CONTENTS_SOLID)
				goto realcheck;
		}

	return true;        // we got out easy

realcheck:
//
// check it for real...
//

// the midpoint must be within 16 of the bottom
	start.x = stop.x = (cmins.x + cmaxs.x) * 0.5f;
	start.y = stop.y = (cmins.y + cmaxs.y) * 0.5f;

#ifdef ROGUE_AI
	if (ent.gravityVector[2] > 0)
	{
		start.z = cmaxs.z;
		stop.z = start.z + 2 * STEPSIZE;
	}
	else
	{
#endif
		start.z = cmins.z;
		stop.z = start.z - 2 * STEPSIZE;
#ifdef ROGUE_AI
	}
#endif
	trace = gi.traceline(start, stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0f)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for (x = 0 ; x <= 1 ; x++)
		for (y = 0 ; y <= 1 ; y++) {
			start.x = stop.x = x ? cmaxs.x : cmins.x;
			start.y = stop.y = y ? cmaxs.y : cmins.y;

			trace = gi.traceline(start, stop, ent, MASK_MONSTERSOLID);

#ifdef ROGUE_AI
			if (ent.gravityVector[2] > 0)
			{
				if (trace.fraction != 1.0 && trace.endpos[2] < bottom)
					bottom = trace.endpos[2];
				if (trace.fraction == 1.0 || trace.endpos[2] - mid > STEPSIZE)
					return false;
			}
			else
			{
#endif
				if (trace.fraction != 1.0f && trace.endpos[2] > bottom)
					bottom = trace.endpos[2];
				if (trace.fraction == 1.0f || mid - trace.endpos[2] > STEPSIZE)
					return false;

#ifdef ROGUE_AI
			}
#endif
		}

	return true;
}

// temp
static entity_type ET_MONSTER_CARRIER, ET_MONSTER_WIDOW, ET_MONSTER_WIDOW2;

static bool SV_moveswimfly(entity &ent, vector move, bool relink, vector oldorg)
{
	// try one move with vertical motion, then one without
	for (int32_t i = 0 ; i < 2 ; i++)
	{
		vector neworg = ent.s.origin + move;

		if (i == 0 && ent.enemy.has_value())
		{
			if (!ent.goalentity.has_value())
				ent.goalentity = ent.enemy;

			float dz = ent.s.origin[2] - ent.goalentity->s.origin[2];

			if (ent.goalentity->is_client())
			{
#ifdef GROUND_ZERO
				// we want the carrier to stay a certain distance off the ground, to help prevent him
				// from shooting his fliers, who spawn in below him
				const float minheight = (ent.type == ET_MONSTER_CARRIER) ? 104.f : 40.f;
#else
				constexpr float minheight = 40.f;
#endif

				if (dz > minheight)
					neworg.z -= 8;

				if (!((ent.flags & FL_SWIM) && ent.waterlevel < 2))
					if (dz < (minheight - 10))
						neworg.z += 8;
			}
			else
			{
#ifdef THE_RECKONING
				// RAFAEL
				if (ent.type == ET_MONSTER_FIXBOT)
				{
					if (ent.s.frame >= 105 && ent.s.frame <= 120)
					{
						if (dz > 12)
							neworg.z--;
						else if (dz < -12)
							neworg.z++;
					}
					else if (ent.s.frame >= 31 && ent.s.frame <= 88)
					{
						if (dz > 12)
							neworg.z -= 12;
						else if (dz < -12)
							neworg.z += 12;
					}
					else
					{
						if (dz > 12)
							neworg.z -= 8;
						else if (dz < -12)
							neworg.z += 8;
					}
				}
				// RAFAEL ( else )
				else
				{
#endif
					if (dz > 8)
						neworg.z -= 8;
					else if (dz > 0)
						neworg.z -= dz;
					else if (dz < -8)
						neworg.z += 8;
					else
						neworg.z += dz;
#ifdef THE_RECKONING
				}
#endif
			}
		}
		
		trace trace = gi.trace(ent.s.origin, ent.mins, ent.maxs, neworg, ent, MASK_MONSTERSOLID);

		// fly monsters don't enter water voluntarily
		if ((ent.flags & FL_FLY) && !ent.waterlevel)
		{
			vector test;
			test.x = trace.endpos[0];
			test.y = trace.endpos[1];
			test.z = trace.endpos[2] + ent.mins.z + 1;
			content_flags contents = gi.pointcontents(test);

			if (contents & MASK_WATER)
				return false;
		}
		// swim monsters don't exit water voluntarily
		else if ((ent.flags & FL_SWIM) && ent.waterlevel < 2)
		{
			vector test;
			test.x = trace.endpos[0];
			test.y = trace.endpos[1];
			test.z = trace.endpos[2] + ent.mins.z + 1;
			content_flags contents = gi.pointcontents(test);

			if (!(contents & MASK_WATER))
				return false;
		}

		if (!trace.allsolid && !trace.startsolid)
		{
			ent.s.origin = trace.endpos;

			if (trace.fraction < 1)
				SV_Impact(ent, trace);

#ifdef ROGUE_AI
			if (!ent.bad_area.has_value() && CheckForBadArea(ent).has_value())
				ent.s.origin = oldorg;
			else
			{
#endif
				if (relink)
				{
					gi.linkentity(ent);
					G_TouchTriggers(ent);
				}
				return true;
#ifdef ROGUE_AI
			}
#endif
		}

		if (!ent.enemy.has_value())
			break;
	}

	return false;
}

bool SV_movestep(entity &ent, vector move, bool relink)
{
	float	dz;
	vector	oldorg, neworg, end;
	trace	trace;
	int	i;
	float	stepsize;
	vector	test;
	int	contents;
#ifdef ROGUE_AI
	entityref	current_bad;

	// PMM - who cares about bad areas if you're dead?
	if (ent.health > 0)
	{
		current_bad = CheckForBadArea(ent);

		if(current_bad.has_value())
		{
			ent.bad_area = current_bad;
			 
#ifdef GROUND_ZERO
			if (ent.enemy.has_value() && ent.enemy->type == ET_TESLA)
			{
				// if the tesla is in front of us, back up...
				if (IsBadAhead (ent, current_bad, move))
					move = -move;
			}
#endif
		}
		else if (ent.bad_area.has_value())
		{
			// if we're no longer in a bad area, get back to business.
			ent.bad_area = 0;
			if (ent.oldenemy.has_value())
			{
				ent.enemy = ent.oldenemy;
				ent.goalentity = ent.oldenemy;
				FoundTarget(ent);
				return true;
			}
		}
	}
#endif

// try the move
	oldorg = ent.s.origin;
	neworg = ent.s.origin + move;

// flying monsters don't step up
	if (ent.flags & (FL_SWIM | FL_FLY))
		return SV_moveswimfly(ent, move, relink, oldorg);

// push down from a step height above the wished position
	if (!(ent.monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1.f;

#ifdef ROGUE_AI
	// trace from 1 stepsize gravityUp to 2 stepsize gravityDown.
	neworg += ent.gravityVector * (-1 * stepsize);
	end = neworg + (ent.gravityVector * (2 * stepsize));
#else
	neworg.z += stepsize;
	end = neworg;
	end.z -= stepsize * 2;
#endif

	trace = gi.trace(neworg, ent.mins, ent.maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg.z -= stepsize;
		trace = gi.trace(neworg, ent.mins, ent.maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}

	// don't go into deep water
	if (ent.waterlevel <= 1)
	{
		test.x = trace.endpos[0];
		test.y = trace.endpos[1];
#ifdef ROGUE_AI
		if(ent.gravityVector[2] > 0)
			test[2] = trace.endpos[2] + ent.maxs[2] - 27;	
		else
#endif
			test.z = trace.endpos[2] + ent.mins.z + 27;
		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if (ent.flags & FL_PARTIALGROUND)
		{
			ent.s.origin += move;
			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}
			ent.groundentity = null_entity;
			return true;
		}

		return false;       // walked off an edge
	}

	if (trace.fraction < 1)
		SV_Impact(ent, trace);

// check point traces down for dangling corners
	ent.s.origin = trace.endpos;
	
#ifdef ROGUE_AI
	// PMM - don't bother with bad areas if we're dead
	if (ent.health > 0)
	{
		// use AI_BLOCKED to tell the calling layer that we're now mad at a tesla
		new_bad = CheckForBadArea(ent);

		if(!current_bad.has_value() && new_bad.has_value())
		{
#ifdef GROUND_ZERO
			if (new_bad->owner.has_value() && new_bad->owner->type == ET_TESLA)
			{
				if (!ent.enemy.has_value() || !ent.enemy->inuse)
				{
					TargetTesla (ent, new_bad->owner);
					ent.monsterinfo.aiflags |= AI_BLOCKED;
				}
				else if (ent.enemy->type == ET_TESLA)
				{
					if (ent.enemy.has_value() && ent.enemy->is_client())
					{
						if (!visible(ent, ent.enemy))
						{
							TargetTesla (ent, new_bad->owner);
							ent.monsterinfo.aiflags |= AI_BLOCKED;
						}
					}
					else
					{
						TargetTesla (ent, new_bad->owner);
						ent.monsterinfo.aiflags |= AI_BLOCKED;
					}
				}
			}
#endif

			ent.s.origin = oldorg;
			return false;
		}
	}
#endif

	if (!M_CheckBottom(ent))
	{
		if (ent.flags & FL_PARTIALGROUND)
		{
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}
			return true;
		}
		ent.s.origin = oldorg;
		return false;
	}

	ent.flags &= ~FL_PARTIALGROUND;
	ent.groundentity = trace.ent;
	ent.groundentity_linkcount = trace.ent.linkcount;

// the move is ok
	if (relink)
	{
		gi.linkentity(ent);
		G_TouchTriggers(ent);
	}

	return true;
}

//============================================================================

void M_ChangeYaw(entity &ent)
{
	float	ideal;
	float	current;
	float	move;
	float	speed;

	current = anglemod(ent.s.angles[YAW]);
	ideal = ent.ideal_yaw;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = ent.yaw_speed;
	if (ideal > current) {
		if (move >= 180)
			move = move - 360;
	} else {
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0) {
		if (move > speed)
			move = speed;
	} else {
		if (move < -speed)
			move = -speed;
	}

	ent.s.angles[YAW] = anglemod(current + move);
}


/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
static bool SV_StepDirection(entity &ent, float yaw, float dist)
{
	if(!ent.inuse)
		return true;
	
	vector	move, oldorigin;
	float	delta;

	ent.ideal_yaw = yaw;
	M_ChangeYaw(ent);

	yaw = deg2rad(yaw);
	move.x = cos(yaw) * dist;
	move.y = sin(yaw) * dist;
	move.z = 0;

	oldorigin = ent.s.origin;
	if (SV_movestep(ent, move, false))
	{
		if(!ent.inuse)
			return true;

#ifdef ROGUE_AI
		ent.monsterinfo.aiflags &= ~AI_BLOCKED;
#endif
		
		delta = ent.s.angles[YAW] - ent.ideal_yaw;
		
#ifdef GROUND_ZERO
		if (ent.type != ET_MONSTER_WIDOW && ent.type != ET_MONSTER_WIDOW2)
#endif
			if (delta > 45 && delta < 315)
			{
				// not turned far enough, so don't take the step
				ent.s.origin = oldorigin;
			}

		gi.linkentity(ent);
		G_TouchTriggers(ent);
		return true;
	}
	gi.linkentity(ent);
	G_TouchTriggers(ent);
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
static void SV_FixCheckBottom(entity &ent)
{
	ent.flags |= FL_PARTIALGROUND;
}



static constexpr float DI_NODIR	= -1;

void SV_NewChaseDir(entity &actor, entityref enemy, float dist)
{
	float   deltax, deltay;
	vector	d;
	float   tdir, olddir, turnaround;

	//FIXME: how did we get here with no enemy
	if (!enemy.has_value())
		return;

	olddir = anglemod((int)(actor.ideal_yaw / 45) * 45);
	turnaround = anglemod(olddir - 180);

	deltax = enemy->s.origin[0] - actor.s.origin[0];
	deltay = enemy->s.origin[1] - actor.s.origin[1];
	if (deltax > 10)
		d.y = 0;
	else if (deltax < -10)
		d.y = 180.f;
	else
		d.y = DI_NODIR;
	if (deltay < -10)
		d.z = 270.f;
	else if (deltay > 10)
		d.z = 90.f;
	else
		d.z = DI_NODIR;

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR) {
		if (d.y == 0)
			tdir = d.z == 90.f ? 45.f : 315.f;
		else
			tdir = d.z == 90.f ? 135.f : 215.f;

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

// try other directions
	if (((Q_rand() & 3) & 1) || fabs(deltay) > fabs(deltax)) {
		tdir = d.y;
		d.y = d.z;
		d.z = tdir;
	}

	if (d.y != DI_NODIR && d.y != turnaround
		&& SV_StepDirection(actor, d.y, dist))
		return;

	if (d.z != DI_NODIR && d.z != turnaround
		&& SV_StepDirection(actor, d.z, dist))
		return;

#ifdef ROGUE_AI
	if (actor.monsterinfo.blocked && actor.inuse && actor.health > 0 && actor.monsterinfo.blocked(actor, dist))
		return;
#endif

	/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, olddir, dist))
		return;

	if (Q_rand() & 1) { /*randomly determine direction of search*/
		for (tdir = 0 ; tdir <= 315.f ; tdir += 45.f)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	} else {
		for (tdir = 315.f ; tdir >= 0 ; tdir -= 45.f)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist))
		return;

	actor.ideal_yaw = olddir;      // can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!M_CheckBottom(actor))
		SV_FixCheckBottom(actor);
}

/*
======================
SV_CloseEnough

======================
*/
static inline bool SV_CloseEnough(entity &ent, entity &goal, const float &dist)
{
	if (goal.absmin.x > ent.absmax.x + dist)
		return false;
	if (goal.absmax.x < ent.absmin.x - dist)
		return false;
	if (goal.absmin.y > ent.absmax.y + dist)
		return false;
	if (goal.absmax.y < ent.absmin.y - dist)
		return false;
	if (goal.absmin.z > ent.absmax.z + dist)
		return false;
	if (goal.absmax.z < ent.absmin.z - dist)
		return false;
	return true;
}


void M_MoveToGoal(entity &ent, float dist)
{
	entityref goal = ent.goalentity;

	if (!ent.groundentity.has_value() && !(ent.flags & (FL_FLY | FL_SWIM)))
		return;

// if the next step hits the enemy, return immediately
	if (ent.enemy.has_value() && SV_CloseEnough(ent, ent.enemy, dist))
		return;

// bump around...
	if (
#ifdef ROGUE_AI
		((Q_rand() & 3) == 1 && !(ent.monsterinfo.aiflags & AI_CHARGING))
#else
		(Q_rand() & 3) == 1
#endif
		|| !SV_StepDirection(ent, ent.ideal_yaw, dist))
	{
#ifdef ROGUE_AI
		if (ent.monsterinfo.aiflags & AI_BLOCKED)
		{
			ent.monsterinfo.aiflags &= ~AI_BLOCKED;
			return;
		}
#endif
		
		if (ent.inuse)
			SV_NewChaseDir(ent, goal, dist);
	}
}


bool M_walkmove(entity &ent, float yaw, float dist)
{
	vector	move;

	if (ent.groundentity == null_entity && !(ent.flags & (FL_FLY | FL_SWIM)))
		return false;

	yaw = deg2rad(yaw);
	move.x = cos(yaw) * dist;
	move.y = sin(yaw) * dist;
	move.z = 0;

#ifdef ROGUE_AI
	bool retval = SV_movestep(ent, move, true);
	ent.monsterinfo.aiflags &= ~AI_BLOCKED;
	return retval;
#else
	return SV_movestep(ent, move, true);
#endif
}

#endif
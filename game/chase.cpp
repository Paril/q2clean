#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "chase.h"
#include "game.h"

void UpdateChaseCam(entity &ent)
{
	entityref targ = ent.client->g.chase_target;

	// is our chase target gone?
	if (!targ->inuse || targ->client->g.resp.spectator)
	{
		ChaseNext(ent);

		if (ent.client->g.chase_target == targ)
		{
			ent.client->g.chase_target = nullptr;
			ent.client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			return;
		}

		targ = ent.client->g.chase_target;
	}

	vector ownerv = targ->s.origin;
	ownerv.z += targ->g.viewheight;

	vector angles = targ->client->g.v_angle;
	if (angles[PITCH] > 56)
		angles[PITCH] = 56.f;

	vector forward, right;
	AngleVectors(angles, &forward, &right, nullptr);
	VectorNormalize(forward);
	vector o = ownerv + (-30 * forward);

	if (o.z < targ->s.origin[2] + 20.f)
		o.z = targ->s.origin[2] + 20.f;

	// jump animation lifts
	if (!targ->g.groundentity.has_value())
		o.z += 16;

	trace tr = gi.traceline(ownerv, o, targ, MASK_SOLID);

	vector goal = tr.endpos + (2 * forward);

	// pad for floors and ceilings
	o = goal;
	o.z += 6;
	tr = gi.traceline(goal, o, targ, MASK_SOLID);
	if (tr.fraction < 1)
	{
		goal = tr.endpos;
		goal.z -= 6;
	}

	o = goal;
	o.z -= 6;
	tr = gi.traceline(goal, o, targ, MASK_SOLID);
	if (tr.fraction < 1)
	{
		goal = tr.endpos;
		goal.z += 6;
	}

	if (targ->g.deadflag)
		ent.client->ps.pmove.pm_type = PM_DEAD;
	else
		ent.client->ps.pmove.pm_type = PM_FREEZE;

	ent.s.origin = goal;
	ent.client->ps.pmove.set_delta_angles(targ->client->g.v_angle - ent.client->g.resp.cmd_angles);

	if (targ->g.deadflag)
	{
		ent.client->ps.viewangles[ROLL] = 40.f;
		ent.client->ps.viewangles[PITCH] = -15.f;
		ent.client->ps.viewangles[YAW] = targ->client->g.killer_yaw;
	}
	else
	{
		ent.client->ps.viewangles = targ->client->g.v_angle;
		ent.client->g.v_angle = targ->client->g.v_angle;
	}

	ent.g.viewheight = 0;
	ent.client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);

	if (ent.client->g.update_chase || (!ent.client->g.showscores &&
#ifdef PMENU
		!ent.client.menu.open &&
#endif
		!ent.client->g.showinventory &&
#ifdef SINGLE_PLAYER
		!ent.client.showhelp &&
#endif
		!(level.framenum & 31)))
	{
		ent.client->g.update_chase = false;
		gi.WriteByte(svc_layout);
		gi.WriteString(strconcat("xv 0 yb -68 string2 \"Chasing ", targ->client->g.pers.netname, "\""));
		gi.unicast(ent, false);
	}
}

void ChaseNext(entity &ent)
{
	if (!ent.client->g.chase_target.has_value())
		return;

	uint32_t i = ent.client->g.chase_target->s.number;
	entityref e;

	do
	{
		i++;
		if (i > game.maxclients)
			i = 1;
		e = itoe(i);
		if (!e->inuse)
			continue;
		if (!e->client->g.resp.spectator)
			break;
	} while (e != ent.client->g.chase_target);

	ent.client->g.chase_target = e;
	ent.client->g.update_chase = true;
}

void ChasePrev(entity &ent)
{
	if (!ent.client->g.chase_target.has_value())
		return;

	uint32_t i = ent.client->g.chase_target->s.number;
	entityref e;
	
	do
	{
		i--;
		if (i < 1)
			i = game.maxclients;
		e = itoe(i);
		if (!e->inuse)
			continue;
		if (!e->client->g.resp.spectator)
			break;
	} while (e != ent.client->g.chase_target);
	
	ent.client->g.chase_target = e;
	ent.client->g.update_chase = true;
}

void GetChaseTarget(entity &ent)
{
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		entity &other = itoe(i);
		
		if (other.inuse && !other.client->g.resp.spectator)
		{
			ent.client->g.chase_target = other;
			ent.client->g.update_chase = true;
			UpdateChaseCam(ent);
			return;
		}
	}

	gi.centerprintf(ent, "No other players to chase.");
}

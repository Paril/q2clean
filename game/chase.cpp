#include "config.h"
#include "entity.h"
#include "chase.h"
#include "game.h"
#include "lib/gi.h"
#include "game.h"

void UpdateChaseCam(entity &ent)
{
	entityref targ = ent.client->chase_target;

	// is our chase target gone?
	if (!targ->inuse || targ->client->resp.spectator)
	{
		ChaseNext(ent);

		if (ent.client->chase_target == targ)
		{
			ent.client->chase_target = nullptr;
			ent.client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			return;
		}

		targ = ent.client->chase_target;
	}

	vector ownerv = targ->s.origin;
	ownerv.z += targ->viewheight;

	vector angles = targ->client->v_angle;
	if (angles[PITCH] > 56)
		angles[PITCH] = 56.f;

	vector forward, right;
	AngleVectors(angles, &forward, &right, nullptr);
	VectorNormalize(forward);
	vector o = ownerv + (-30 * forward);

	if (o.z < targ->s.origin[2] + 20.f)
		o.z = targ->s.origin[2] + 20.f;

	// jump animation lifts
	if (!targ->groundentity.has_value())
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

	if (targ->deadflag)
		ent.client->ps.pmove.pm_type = PM_DEAD;
	else
		ent.client->ps.pmove.pm_type = PM_FREEZE;

	ent.s.origin = goal;
	ent.client->ps.pmove.set_delta_angles(targ->client->v_angle - ent.client->resp.cmd_angles);

	if (targ->deadflag)
	{
		ent.client->ps.viewangles[ROLL] = 40.f;
		ent.client->ps.viewangles[PITCH] = -15.f;
		ent.client->ps.viewangles[YAW] = targ->client->killer_yaw;
	}
	else
	{
		ent.client->ps.viewangles = targ->client->v_angle;
		ent.client->v_angle = targ->client->v_angle;
	}

	ent.viewheight = 0;
	ent.client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);

	if (ent.client->update_chase || (!ent.client->showscores &&
#ifdef PMENU
		!ent.client.menu.open &&
#endif
		!ent.client->showinventory &&
#ifdef SINGLE_PLAYER
		!ent.client->showhelp &&
#endif
		!(level.framenum & 31)))
	{
		ent.client->update_chase = false;
		gi.WriteByte(svc_layout);
		gi.WriteString(strconcat("xv 0 yb -68 string2 \"Chasing ", targ->client->pers.netname, "\""));
		gi.unicast(ent, false);
	}
}

void ChaseNext(entity &ent)
{
	if (!ent.client->chase_target.has_value())
		return;

	uint32_t i = ent.client->chase_target->s.number;
	entityref e;

	do
	{
		i++;
		if (i > game.maxclients)
			i = 1;
		e = itoe(i);
		if (!e->inuse)
			continue;
		if (!e->client->resp.spectator)
			break;
	} while (e != ent.client->chase_target);

	ent.client->chase_target = e;
	ent.client->update_chase = true;
}

void ChasePrev(entity &ent)
{
	if (!ent.client->chase_target.has_value())
		return;

	uint32_t i = ent.client->chase_target->s.number;
	entityref e;
	
	do
	{
		i--;
		if (i < 1)
			i = game.maxclients;
		e = itoe(i);
		if (!e->inuse)
			continue;
		if (!e->client->resp.spectator)
			break;
	} while (e != ent.client->chase_target);
	
	ent.client->chase_target = e;
	ent.client->update_chase = true;
}

void GetChaseTarget(entity &ent)
{
	for (entity &other : entity_range(1, game.maxclients))
	{
		if (other.inuse && !other.client->resp.spectator)
		{
			ent.client->chase_target = other;
			ent.client->update_chase = true;
			UpdateChaseCam(ent);
			return;
		}
	}

	gi.centerprintf(ent, "No other players to chase.");
}

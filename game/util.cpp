#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/gi.h"
#include "game.h"
#include "util.h"
#include "combat.h"

class bad_entity_operation : public std::exception
{
public:
	bad_entity_operation(stringlit msg) :
		std::exception(msg, 1)
	{
	}
};

void G_InitEdict(entity &e)
{
	if (e.inuse)
		throw bad_entity_operation("entity already in use");

	e.__init();
	e.inuse = true;
	e.g.gravity = 1.0f;
#ifdef GROUND_ZERO
	e.g.gravityVector = MOVEDIR_DOWN;
#endif
}

entity &G_Spawn()
{
	size_t i;
	
	for (i = game.maxclients + 1; i < num_entities; i++)
	{
		entity &e = itoe(i);

		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e.inuse && (e.g.freeframenum < (2 * BASE_FRAMERATE) || (level.framenum - e.g.freeframenum) > (gtime)(0.5f * BASE_FRAMERATE)))
		{
		    G_InitEdict(e);
		    return e;
		}
	}

	if (i == max_entities)
		gi.error("%s: no free edicts", __func__);

	entity &e = itoe(i);
	num_entities++;
	G_InitEdict(e);

	return e;
}

void G_FreeEdict(entity &e)
{
	if (!e.inuse)
		throw bad_entity_operation("entity not in use");
	else if (e.s.number <= (game.maxclients + BODY_QUEUE_SIZE))
		throw bad_entity_operation("entity is reserved; cannot free");

	gi.unlinkentity(e);        // unlink from world
	
	e.__free();
	e.inuse = false;
	e.g.type = ET_FREED;
	e.g.freeframenum = level.framenum;
}

entityref findradius(entityref from, vector org, float rad)
{
	if (!from.has_value() || from->is_world())
		from = itoe(1);
	else
		from = next_ent(from);

	for (; etoi(from) < num_entities; from = next_ent(from))
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		vector eorg = org - (from->s.origin + (from->mins + from->maxs) * 0.5f);
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return null_entity;
}

entityref G_PickTarget(const stringref &stargetname)
{
	if (!stargetname)
	{
		gi.dprintf("G_PickTarget called with empty targetname\n");
		return nullptr;
	}

	entityref			ent;
	dynarray<entityref>	choice;

	while ((ent = G_FindFunc(ent, g.targetname, stargetname, striequals)).has_value())
		choice.push_back(ent);

	if (!choice.size())
	{
		gi.dprintf("G_PickTarget: target %s not found\n", stargetname.ptr());
		return nullptr;
	}

	return choice[Q_rand_uniform(choice.size())];
}

static void Think_Delay(entity &ent)
{
	G_UseTargets(ent, ent.g.activator);
	G_FreeEdict(ent);
}

void G_UseTargets(entity &ent, entity &cactivator)
{
//
// check for a delay
//
	if (ent.g.delay)
	{
		// create a temp object to fire at a later time
		entity &t = G_Spawn();
		t.g.type = ET_DELAYED_USE;
		t.g.nextthink = level.framenum + (gtime)(ent.g.delay * BASE_FRAMERATE);
		t.g.think = Think_Delay;
		t.g.activator = cactivator;
		t.g.message = ent.g.message;
		t.g.target = ent.g.target;
		t.g.killtarget = ent.g.killtarget;
		return;
	}

//
// print the message
//
	if ((ent.g.message) && !(cactivator.svflags & SVF_MONSTER))
	{
		gi.centerprintf(cactivator, "%s", ent.g.message.ptr());
		if (ent.g.noise_index)
			gi.sound(cactivator, CHAN_AUTO, ent.g.noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent.g.killtarget)
	{
#ifdef GROUND_ZERO
		bool done = false;
		entity master;
#endif
		entityref t;

		while ((t = G_FindFunc(t, g.targetname, ent.g.killtarget, striequals)).has_value())
		{
#ifdef GROUND_ZERO
			// PMM - if this entity is part of a train, cleanly remove it
			if ((t.flags & FL_TEAMSLAVE) && t.teammaster)
			{
				master = t.teammaster;
				while (!done)
				{
					if (master.teamchain == t)
					{
						master.teamchain = t.teamchain;
						done = true;
					}
					master = master.teamchain;
				}
			}
#endif

			G_FreeEdict(t);

			if (!ent.inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

//
// fire targets
//
	if (ent.g.target)
	{
		entityref t;

		while ((t = G_FindFunc(t, g.targetname, ent.g.target, striequals)).has_value())
		{
			// doors fire area portals in a specific way
			if (t->g.type == ET_FUNC_AREAPORTAL &&
				(ent.g.type == ET_FUNC_DOOR || ent.g.type == ET_FUNC_DOOR_ROTATING))
				continue;

			if (t == ent)
				gi.dprintf("WARNING: Entity used itself.\n");
			else if (t->g.use)
				t->g.use(t, ent, cactivator);

			if (!ent.inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}

constexpr vector VEC_UP		= { 0, -1, 0 };
constexpr vector VEC_DOWN	= { 0, -2, 0 };

void G_SetMovedir(vector &angles, vector &movedir)
{
	if (angles == VEC_UP)
		movedir = MOVEDIR_UP;
	else if (angles == VEC_DOWN)
		movedir = MOVEDIR_DOWN;
	else
		AngleVectors(angles, &movedir, nullptr, nullptr);

	angles = vec3_origin;
}

#ifdef CTF
// from ctf.qc
void() CTFResetFlags;
void(ctfteam_t) CTFResetFlag;
void(entity) CTFRespawnTech;
string(ctfteam_t) CTFTeamName;
#endif

void BecomeExplosion1(entity &self)
{
#ifdef CTF
	if (self.item)
	{
		// techs are important
		if (self.item->flags & IT_TECH)
		{
			CTFRespawnTech(self); // this frees self!
			return;
		}
		//flags are important
		else if (self.item->id == ITEM_FLAG1)
		{
			CTFResetFlag(CTF_TEAM1); // this will free self!
			gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(CTF_TEAM1));
			return;
		}
		else if (self.item->id == ITEM_FLAG2)
		{
			CTFResetFlag(CTF_TEAM2); // this will free self!
			gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(CTF_TEAM1));
			return;
		}
	}
#endif
	
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self.s.origin);
	gi.multicast(self.s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void BecomeExplosion2(entity &self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self.s.origin);
	gi.multicast(self.s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void G_TouchTriggers(entity &ent)
{
	// dead things don't activate triggers!
	if ((ent.is_client() || (ent.svflags & SVF_MONSTER)) && (ent.g.health <= 0))
		return;

	dynarray<entityref> touches = gi.BoxEdicts(ent.absmin, ent.absmax, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (entity &hit : touches)
	{
		if (!hit.inuse)
			continue;
		if (!hit.g.touch)
			continue;
		hit.g.touch(hit, ent, vec3_origin, null_surface);
	}
}

bool KillBox(entity &ent)
{
	while (1)
	{
		trace tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, ent.s.origin, world, MASK_PLAYERSOLID);

		if (tr.ent.is_world())
			break;

		// nail it
		T_Damage(tr.ent, ent, ent, vec3_origin, ent.s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent.solid)
			return false;
	}

	return true;        // all clear
};

bool visible(const entity &self, const entity &other)
{
	vector spot1 = self.s.origin;
	spot1.z += self.g.viewheight;
	vector spot2 = other.s.origin;
	spot2.z += other.g.viewheight;
	trace tr = gi.traceline(spot1, spot2, self, MASK_OPAQUE);
	
	if (tr.fraction == 1.0f || tr.ent == other)
		return true;
	return false;
}

bool infront(const entity &self, const entity &other)
{
	vector forward;
	AngleVectors(self.s.angles, &forward, nullptr, nullptr);
	vector vec = other.s.origin - self.s.origin;
	VectorNormalize(vec);

	return (vec * forward) > 0.3f;
}

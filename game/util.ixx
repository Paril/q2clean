module;

#include "../lib/types.h"
#include "game.h"
#include "entity.h"
#include "combat.h"

export module util;

import math.random;
import dynarray;

export constexpr vector MOVEDIR_UP		= { 0, 0, 1 };
export constexpr vector MOVEDIR_DOWN	= { 0, 0, -1 };

export constexpr size_t BODY_QUEUE_SIZE = 8;

/*
=================
G_InitEdict

Marks an entity as active, and sets up some default parameters.
=================
*/
export void G_InitEdict(entity &e);

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
export entity &G_Spawn();

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
export void G_FreeEdict(entity &e);

export DECLARE_SAVABLE_FUNCTION(G_FreeEdict);

export constexpr vector G_ProjectSource(const vector &point, const vector &distance, const vector &forward, const vector &right)
{
	return point + (forward * distance.x) + (right * distance.y) + vector(0, 0, distance.z);
}

import gi;
import game_locals;

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching member in the structure.

Searches beginning at the edict after from, or the beginning (after clients) if
from is null/world/a client. null_entity will be returned if the end of the list is reached.
=============
*/
export template<typename T, typename C>
inline entityref G_Find(entityref from, const T &match, C matcher)
{
	if (!from.has_value() || etoi(from) <= game.maxclients)
		from = itoe(game.maxclients + 1);
	else
		from = next_ent(from);

	for (; etoi(from) < num_entities; from = next_ent(from))
		if (from->inuse && matcher(from, match))
			return from;

	return null_entity;
}

template<auto member>
using entity_field_type = std::decay_t<decltype(std::declval<entity>().*member)>;

// G_Find using the specified function to match the specified entity member.
export template<auto member>
inline entityref G_FindFunc(entityref from, const entity_field_type<member> &match, bool (*matcher)(const entity_field_type<member> &a, const entity_field_type<member> &b))
{
	return G_Find<entity_field_type<member>>(from, match, [matcher](const entity &e, const entity_field_type<member> &m) { return matcher(e.*member, m); });
}

// G_Find using a simple wrapper to the == operator of the specified member
export template<auto member>
inline entityref G_FindEquals(entityref from, const entity_field_type<member> &match)
{
	return G_Find<entity_field_type<member>>(from, match, [](const entity &e, const entity_field_type<member> &m) { return e.*member == m; });
}

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
export entityref findradius(entityref from, vector org, float rad);

/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if world.
null_entity will be returned if the end of the list is reached.
=============
*/
export entityref G_PickTarget(const stringref &stargetname);

/*
==============================
G_UseTargets

"cactivator" should be set to the entity that initiated the firing.

If ent.delay is set, a DelayedUse entity will be created that will actually
do the G_UseTargets after that many seconds have passed.

Centerprints any ent.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function
==============================
*/
export void G_UseTargets(entity &ent, entity &cactivator);

export void G_SetMovedir(vector &angles, vector &movedir);

export void BecomeExplosion1(entity &self);

export void BecomeExplosion2(entity &self);

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
export bool KillBox(entity &ent);

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
export bool visible(const entity &self, const entity &other);

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
export bool infront(const entity &self, const entity &other);

export void G_TouchTriggers(entity &ent);

module :private;

import game_locals;

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
	e.gravity = 1.0f;
#ifdef GROUND_ZERO
	e.gravityVector = MOVEDIR_DOWN;
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
		if (!e.inuse && (e.freeframenum < (2 * BASE_FRAMERATE) || (level.framenum - e.freeframenum) >(gtime)(0.5f * BASE_FRAMERATE)))
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
	e.type = ET_FREED;
	e.freeframenum = level.framenum;
}

REGISTER_SAVABLE_FUNCTION(G_FreeEdict);

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

	while ((ent = G_FindFunc<&entity::targetname>(ent, stargetname, striequals)).has_value())
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
	G_UseTargets(ent, ent.activator);
	G_FreeEdict(ent);
}

REGISTER_SAVABLE_FUNCTION(Think_Delay);

void G_UseTargets(entity &ent, entity &cactivator)
{
	//
	// check for a delay
	//
	if (ent.delay)
	{
		// create a temp object to fire at a later time
		entity &t = G_Spawn();
		t.type = ET_DELAYED_USE;
		t.nextthink = level.framenum + (gtime) (ent.delay * BASE_FRAMERATE);
		t.think = Think_Delay_savable;
		t.activator = cactivator;
		t.message = ent.message;
		t.target = ent.target;
		t.killtarget = ent.killtarget;
		return;
	}

	//
	// print the message
	//
	if ((ent.message) && !(cactivator.svflags & SVF_MONSTER))
	{
		gi.centerprintf(cactivator, "%s", ent.message.ptr());
		if (ent.noise_index)
			gi.sound(cactivator, CHAN_AUTO, ent.noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound(cactivator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	//
	// kill killtargets
	//
	if (ent.killtarget)
	{
#ifdef GROUND_ZERO
		bool done = false;
		entity master;
#endif
		entityref t;

		while ((t = G_FindFunc<&entity::targetname>(t, ent.killtarget, striequals)).has_value())
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
	if (ent.target)
	{
		entityref t;

		while ((t = G_FindFunc<&entity::targetname>(t, ent.target, striequals)).has_value())
		{
			// doors fire area portals in a specific way
			if (t->type == ET_FUNC_AREAPORTAL &&
				(ent.type == ET_FUNC_DOOR || ent.type == ET_FUNC_DOOR_ROTATING))
				continue;

			if (t == ent)
				gi.dprintf("WARNING: Entity used itself.\n");
			else if (t->use)
				t->use(t, ent, cactivator);

			if (!ent.inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}

constexpr vector VEC_UP = { 0, -1, 0 };
constexpr vector VEC_DOWN = { 0, -2, 0 };

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
	if ((ent.is_client() || (ent.svflags & SVF_MONSTER)) && (ent.health <= 0))
		return;

	dynarray<entityref> touches = gi.BoxEdicts(ent.absmin, ent.absmax, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (entity &hit : touches)
	{
		if (!hit.inuse)
			continue;
		if (!hit.touch)
			continue;
		hit.touch(hit, ent, vec3_origin, null_surface);
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
	spot1.z += self.viewheight;
	vector spot2 = other.s.origin;
	spot2.z += other.viewheight;
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

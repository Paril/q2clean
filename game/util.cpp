#include "config.h"
#include "util.h"
#include "lib/types/dynarray.h"
#include "combat.h"
#include "lib/math/random.h"
#include "lib/gi.h"
#include "game.h"
#include "game/func.h"
#include "game/misc.h"

void G_InitEdict(entity &e)
{
	if (e.inuse)
		throw bad_entity_operation("entity already in use");

	e.__init();
	e.inuse = true;
	e.gravity = 1.0f;
#ifdef ROGUE_AI
	e.gravityVector = MOVEDIR_DOWN;
#endif
}

entity &G_Spawn()
{
	// fatal if we ever hit this point
	if (num_entities == max_entities)
		gi.error("%s: reached max entity slots", __func__);

	// guaranteed to have a free entity at the end, but
	// let's try to re-use IDs first
	entityref best;
	// the first couple seconds of server time can involve a lot of
	// freeing and allocating
	const bool quick_replace = level.framenum < 10;

	for (entity &e : entity_range(game.maxclients + 1, num_entities - 1))
	{
		if (e.inuse)
			continue;

		// if this is a really good entity, mark it down immediately
		if (quick_replace || (level.framenum - e.freeframenum) > (gtime)(0.5f * BASE_FRAMERATE))
		{
			best = e;
			break;
		}
		// otherwise we may have no choice but to re-use the first free one we come across
		else if (!best.has_value())
			best = e;
	}

	if (!best.has_value())
		best = itoe(num_entities++);

	G_InitEdict(best);
	return best;
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
	e.type = ET_UNKNOWN;
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

		vector eorg = org - (from->s.origin + from->bounds.center());

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

static REGISTER_SAVABLE_FUNCTION(Think_Delay);

void G_UseTargets(entity &ent, entity &cactivator)
{
	//
	// check for a delay
	//
	if (ent.delay)
	{
		// create a temp object to fire at a later time
		entity &t = G_Spawn();
		t.nextthink = level.framenum + (gtime) (ent.delay * BASE_FRAMERATE);
		t.think = SAVABLE(Think_Delay);
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
#endif
		entityref t;

		while ((t = G_FindFunc<&entity::targetname>(t, ent.killtarget, striequals)).has_value())
		{
#ifdef GROUND_ZERO
			// PMM - if this entity is part of a train, cleanly remove it
			if ((t->flags & FL_TEAMSLAVE) && t->teammaster.has_value())
			{
				entityref master = t->teammaster;

				while (!done)
				{
					if (master->teamchain == t)
					{
						master->teamchain = t->teamchain;
						done = true;
					}

					master = master->teamchain;
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

	dynarray<entityref> touches = gi.BoxEdicts(ent.absbounds, AREA_TRIGGERS);

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
		trace tr = gi.trace(ent.s.origin, ent.bounds, ent.s.origin, world, MASK_PLAYERSOLID);

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

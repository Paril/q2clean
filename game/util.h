#pragma once

#include <exception>
#include "config.h"
#include "game.h"
#include "entity.h"
#include "savables.h"

constexpr vector MOVEDIR_UP		= { 0, 0, 1 };
constexpr vector MOVEDIR_DOWN	= { 0, 0, -1 };

constexpr size_t BODY_QUEUE_SIZE = 8;

class bad_entity_operation : public std::exception
{
public:
	bad_entity_operation(stringlit msg) :
		std::exception(msg, 1)
	{
	}
};

/*
=================
G_InitEdict

Marks an entity as active, and sets up some default parameters.
=================
*/
void G_InitEdict(entity &e);

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
entity &G_Spawn();

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict(entity &e);

DECLARE_SAVABLE(G_FreeEdict);

constexpr vector G_ProjectSource(vector point, vector distance, vector forward, vector right, vector up = { 0, 0, 1 })
{
	return point + (forward * distance.x) + (right * distance.y) + (up * distance.z);
}

// Iterator implementation for enabling iterator-ing through entity chains
#include <iterator>

struct entity_chain_sentinel { };

template<typename T> requires std::is_invocable_r_v<entityref, T, entityref>
struct entity_chain_iterator
{
	T func;
	entityref e;

	entity_chain_iterator() :
		func(nullptr),
		e(null_entity)
	{
	}

	entity_chain_iterator(T func) :
		func(func),
		e(func(nullptr))
	{
	}

	using difference_type = ptrdiff_t;
	using value_type = entity;

	entity &operator*() { return e; }
	entity &operator*() const { return e; }

	entity_chain_iterator &operator++()
	{
		e = func(e);
		return *this;
	}

	entity_chain_iterator operator++(int) const
	{
		entity_chain_iterator it = *this;
		it++;
		return it;
	}

	bool operator==(const entity_chain_iterator &it) const { return e == it.e; }
	bool operator!=(const entity_chain_iterator &it) const { return e != it.e; }

	bool operator==(const entity_chain_sentinel &) const { return e == null_entity; }
	bool operator!=(const entity_chain_sentinel &) const { return e != null_entity; }
};

template<typename T> requires std::is_invocable_r_v<entityref, T, entityref>
struct entity_chain_container
{
	T func;

	entity_chain_container() :
		func(nullptr)
	{
	}

	entity_chain_container(T func) :
		func(func)
	{
	}

	entity_chain_iterator<T> begin() { return entity_chain_iterator(func); }
	entity_chain_sentinel end() { return { }; }
};

template<typename T, typename C>
using is_entity_matcher = std::is_invocable_r<bool, C, entity &, const T &>;

template<typename T, typename C>
constexpr bool is_entity_matcher_v = is_entity_matcher<T, C>::value;

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching member in the structure.

Searches beginning at the edict after from, or the beginning (after clients) if
from is null/world/a client. null_entity will be returned if the end of the list is reached.
=============
*/
template<typename T, typename C> requires is_entity_matcher_v<T, C>
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

template<typename T, typename C> requires is_entity_matcher_v<T, C>
inline auto G_Iterate(const T &match, C matcher)
{
	return entity_chain_container([match, matcher] (entityref e) { return G_Find<T, C>(e, match, matcher); });
}

template<auto member>
using entity_field_type = std::decay_t<decltype(std::declval<entity>().*member)>;

// G_Find using the specified function to match the specified entity member.
template<auto member, typename field = entity_field_type<member>>
inline entityref G_FindFunc(entityref from, const field &match, bool (*matcher)(const field &a, const field &b))
{
	return G_Find<field>(from, match, [matcher](const entity &e, const field &m) { return matcher(e.*member, m); });
}

template<auto member, typename field = entity_field_type<member>>
inline auto G_IterateFunc(const field &match, bool (*matcher)(const field &a, const field &b))
{
	return entity_chain_container([match, matcher] (entityref e) { return G_FindFunc<member>(e, match, matcher); });
}

// G_Find using a simple wrapper to the == operator of the specified member
template<auto member, typename field = entity_field_type<member>>
inline entityref G_FindEquals(entityref from, const field &match)
{
	return G_Find<field>(from, match, [](const entity &e, const field &m) { return e.*member == m; });
}

template<auto member>
inline auto G_IterateEquals(const entity_field_type<member> &match)
{
	return entity_chain_container([match] (entityref e) { return G_FindEquals<member>(e, match); });
}

/*
=================
findradius

Returns entities that have origins within a spherical area
=================
*/
entityref findradius(entityref from, vector org, float rad);

inline auto G_IterateRadius(vector org, float rad)
{
	return entity_chain_container([org, rad] (entityref e) { return findradius(e, org, rad); });
}

/*
=============
G_PickTarget

Pick a random entity that matches the specified targetname.
=============
*/
entityref G_PickTarget(const stringref &stargetname);

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
void G_UseTargets(entity &ent, entity &cactivator);

void G_SetMovedir(vector &angles, vector &movedir);

void BecomeExplosion1(entity &self);

void BecomeExplosion2(entity &self);

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
bool KillBox(entity &ent);

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
bool visible(const entity &self, const entity &other);

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
bool infront(const entity &self, const entity &other);

void G_TouchTriggers(entity &ent);
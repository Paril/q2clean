#pragma once

#include "../lib/types.h"

using spawn_func = void(*)(entity &);

struct registered_entity
{
	stringlit	classname;
	spawn_func	func;
	entity_type	type;
};

// static structure that maintains a list of entities
// we can spawn. we abuse static initializers to make
// this work.
struct spawnable_entities
{
	spawnable_entities(const registered_entity &ent)
	{
		register_entity(ent);
	}

	static void register_entity(const registered_entity &ent);
	
	// iterable-like

	static const registered_entity *begin();
	static const registered_entity *end();
};

#define REGISTER_ENTITY(n, i) \
	static spawnable_entities _ ## n({ .classname = #n, .func = SP_ ## n, .type = i });

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(stringlit mapname, stringlit entities, stringlit spawnpoint);

// Called before SpawnEntities, before entities are wiped.
void PreSpawnEntities();
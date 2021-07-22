#pragma once

import "game/entity_types.h";
import "lib/string.h";

using spawn_func = void(*)(entity &);

struct registered_entity
{
	stringlit	classname;
	spawn_func	func;
	entity_type	type;

	const registered_entity	*next;
};

// static structure that maintains a list of entities
// we can spawn. we abuse static initializers to make
// this work.
struct spawnable_entities
{
private:
	registered_entity ent;

	static void register_entity(registered_entity &ent);

public:
	spawnable_entities(const registered_entity &ent) :
		ent(ent)
	{
		register_entity(this->ent);
	}
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
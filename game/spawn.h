#pragma once

#include "game/entity_types.h"
#include "lib/string.h"

using spawn_func = void(*)(entity &);

struct registered_entity
{
	stringlit	classname;
	spawn_func	func;
	const entity_type	&type;

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

		if (ent.type.spawn)
			throw std::exception("duplicate spawn func");

		*const_cast<const registered_entity **>(&ent.type.spawn) = &this->ent;
	}
};

#define DECLARE_ENTITY(enttype) \
	extern entity_type ET_##enttype

#define REGISTER_ENTITY(enttype, n) \
	entity_type ET_##enttype(#n); \
	static spawnable_entities _##n({ .classname = #n, .func = SP_##n, .type = ET_##enttype })

#define REGISTER_ENTITY_NOSPAWN(enttype, n) \
	entity_type ET_##enttype(#n); \
	static spawnable_entities _##n({ .classname = #n, .func = nullptr, .type = ET_##enttype })

#define REGISTER_ENTITY_INHERIT(enttype, n, inherittype) \
	entity_type ET_##enttype(#n, inherittype); \
	static spawnable_entities _##n({ .classname = #n, .func = SP_##n, .type = ET_##enttype })

stringlit FindClassnameFromEntityType(const entity_type &type);

inline stringlit FindClassnameFromEntityType(const entity_type_ref &type)
{
	return FindClassnameFromEntityType(*type.type);
}

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

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
bool ED_CallSpawn(entity &ent);
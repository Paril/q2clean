#pragma once

#include "types.h"
#include "server_client.h"
#include "entity_effects.h"

// render effects affect the rendering of an entity
enum render_effects : uint32_t
{
	RF_MINLIGHT = 1 << 0,	// always have some light (viewmodel)
	RF_VIEWERMODEL = 1 << 1,	// don't draw through eyes, only mirrors
	RF_WEAPONMODEL = 1 << 2,	// only draw through eyes
	RF_FULLBRIGHT = 1 << 3,	// always draw full intensity
	RF_DEPTHHACK = 1 << 4,	// for view weapon Z crunching
	RF_TRANSLUCENT = 1 << 5,
	RF_FRAMELERP = 1 << 6,
	RF_BEAM = 1 << 7,
	RF_CUSTOMSKIN = 1 << 8,	// skin is an index in image_precache
	RF_GLOW = 1 << 9,		// pulse lighting for bonus items
	RF_SHELL_RED = 1 << 10,
	RF_SHELL_GREEN = 1 << 11,
	RF_SHELL_BLUE = 1 << 12,

//ROGUE
	RF_IR_VISIBLE = 1 << 15,
	RF_SHELL_DOUBLE = 1 << 16,
	RF_SHELL_HALF_DAM = 1 << 17,
	RF_USE_DISGUISE = 1 << 18
//ROGUE
};

MAKE_ENUM_BITWISE(render_effects);

// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
enum entity_event : uint32_t
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
};

// entity state is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
struct entity_state
{
	// edict index; do not touch
	uint32_t	number;

	vector	origin;
	vector	angles;
	// for lerping
	vector	old_origin;

	model_index	modelindex;
	model_index	modelindex2, modelindex3, modelindex4;  // weapons, CTF flags, etc
#ifdef KMQUAKE2_ENGINE_MOD
	model_index	modelindex5, modelindex6;	//more attached models
#endif
	int32_t	frame;
	int32_t	skinnum;
#ifdef KMQUAKE2_ENGINE_MOD
	float	alpha;	//entity transparency
#endif
	entity_effects	effects;
	render_effects	renderfx;
private:
	// solidity value sent over to clients. not used by game DLL.
	int32_t	solid [[maybe_unused]];
public:
	sound_index	sound;			// for looping sounds, to guarantee shutoff
#ifdef KMQUAKE2_ENGINE_MOD // Knightmare- added sound attenuation
	float		attenuation;
#endif
	// impulse events -- muzzle flashes, footsteps, etc
	// events only go out for a single frame, they
	// are automatically cleared each frame
	entity_event	event;
};

// area connection
struct area_list
{
	area_list	*next, *prev;
};

const size_t MAX_ENT_CLUSTERS = 16;

// entity server flags; the engine uses these, so don't change this.
enum server_flags : uint32_t
{
	SVF_NONE,
	// don't send entity to clients, even if it has effects
	SVF_NOCLIENT = 1 << 0,
	// treat as CONTENTS_DEADMONSTER for collision
	SVF_DEADMONSTER = 1 << 1,
	// treat as CONTENTS_MONSTER for collision
	SVF_MONSTER = 1 << 2

#ifdef GROUND_ZERO
	, SVF_DAMAGEABLE = 1 << 3
#endif
};

MAKE_ENUM_BITWISE(server_flags);

enum solidity : uint32_t
{
	// no interaction with other objects
    SOLID_NOT,
	// only touch when inside, after moving
    SOLID_TRIGGER,
	// touch on edge
    SOLID_BBOX,
	// bsp clip, touch on edge
    SOLID_BSP
};

// this is a reference to the world entity.
extern entityref world;

// this is the structure used by all entities in Q2++.
struct server_entity
{
friend struct entity;

private:
	// an error here means you're using entity as a value type. Always use entity&
	// to pass things around.
	server_entity() { }
	// Entities can also not be copied; use a.copy(b)
	// to do a proper copy, which resets members that would break the game.
	server_entity(server_entity&) { };

	// move constructor not allowed; entities can't be "deleted"
	// so move constructor would be weird
	server_entity(server_entity&&) = delete;

public:
	// this data is shared between the game and engine

	entity_state	s;
	client			*client;
	qboolean		inuse;
	int32_t			linkcount;

private:
	// this stuff is private to the engine
	area_list	area;				// linked to a division node or leaf

	int32_t								num_clusters;		// if -1, use headnode instead
	array<int32_t, MAX_ENT_CLUSTERS>	clusternums;

	int32_t		headnode;			// unused if num_clusters != -1

	// back to shared members
public:
	area_index		areanum, areanum2;

	server_flags	svflags;
	vector			mins, maxs;
	vector			absmin, absmax, size;
	solidity		solid;
	content_flags	clipmask;
	entityref		owner;

	// check whether this entity is the world entity.
	// for QC compatibility mostly.
	inline bool is_world() const
	{
		return s.number == 0;
	}

	// check whether this entity is a client and
	// has a client field that can be dereferenced.
	inline bool is_client() const
	{
		return client;
	}

	// check whether this entity is currently linked
	inline bool is_linked() const
	{
		return area.prev;
	}
	
	inline bool operator==(const server_entity &rhs) const { return s.number == rhs.s.number; }
	inline bool operator!=(const server_entity &rhs) const { return s.number != rhs.s.number; }
};

#include "../game/entity.h"
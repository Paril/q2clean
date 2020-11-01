#pragma once

#include "types.h"
#include "../game/gclient.h"
#include "pmove_state.h"

// player_state_t->refdef flags
enum refdef_flags
{
	RDF_NONE,

	RDF_UNDERWATER		= 1 << 0,	// warp the screen as apropriate
	RDF_NOWORLDMODEL	= 1 << 1,	// used for player configuration screen
//ROGUE
	RDF_IRGOGGLES	= 1 << 2,
	RDF_UVGOGGLES	= 1 << 3
//ROGUE
};

MAKE_ENUM_BITWISE(refdef_flags);

// player_state->stats[] indexes
struct player_stat
{
	uint16_t	value;

	player_stat() :
		value(0)
	{
	}

	template<typename T>
	player_stat(const T &v) :
		value((uint16_t)v)
	{
	}

	template<>
	player_stat(const image_index &v) :
		player_stat((int32_t)v)
	{
	}

	inline operator uint16_t &() { return value; }
	inline operator const uint16_t &() const { return value; }
	inline explicit operator image_index() const { return image_index(value); }
};

// For engine compatibility, these 18 IDs should remain the same
// and keep their described usage.
// These are macros so they can be embedded in the static statusbar strings.
#define STAT_HEALTH_ICON	0
#define STAT_HEALTH			1
#define STAT_AMMO_ICON		2
#define STAT_AMMO			3
#define STAT_ARMOR_ICON		4
#define STAT_ARMOR			5
#define STAT_SELECTED_ICON	6
#define STAT_PICKUP_ICON	7
#define STAT_PICKUP_STRING	8
#define STAT_TIMER_ICON		9
#define STAT_TIMER			10
#define STAT_HELPICON		11
#define STAT_SELECTED_ITEM	12
#define STAT_LAYOUTS		13
#define STAT_FRAGS			14
#define STAT_FLASHES		15 // cleared each frame, 1 = health, 2 = armor
#define STAT_CHASE			16
#define STAT_SPECTATOR		17
// Bits from here to (MAX_STATS - 1) are free for mods
#ifdef CTF
#define STAT_CTF_TEAM1_PIC			18
#define STAT_CTF_TEAM1_CAPS			19
#define STAT_CTF_TEAM2_PIC			20
#define STAT_CTF_TEAM2_CAPS			21
#define STAT_CTF_FLAG_PIC			22
#define STAT_CTF_JOINED_TEAM1_PIC	23
#define STAT_CTF_JOINED_TEAM2_PIC	24
#define STAT_CTF_TEAM1_HEADER		25
#define STAT_CTF_TEAM2_HEADER		26
#define STAT_CTF_TECH				27
#define STAT_CTF_ID_VIEW			28
#define STAT_CTF_ID_VIEW_COLOR		29
#define STAT_CTF_TEAMINFO			30
#endif

#ifdef KMQUAKE2_ENGINE_MOD
#define MAX_STATS					256
#else
#define MAX_STATS					32
#endif

// for stringifying stat in static strings
#define STRINGIFY(s) #s
#define STRINGIFY2(s) STRINGIFY(s)
#define STAT(s) STRINGIFY2(STAT_##s)

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
struct player_state
{
	pmove_state	pmove;      // for prediction

	// for fixed views
	vector	viewangles;
	// add to pmovestate->origin
	vector	viewoffset;
	// add to view direction to get render angles
	// set by weapon kicks, pain effects, etc
	vector	kick_angles;

	vector		gunangles;
	vector		gunoffset;
	model_index	gunindex;
	int32_t		gunframe;

#ifdef KMQUAKE2_ENGINE_MOD //Knightmare added
	int32_t		gunskin;		// for animated weapon skins
	model_index	gunindex2;		// for a second weapon model (boot)
	int32_t		gunframe2;
	int32_t		gunskin2;

	// server-side speed control!
	int32_t	maxspeed;
	int32_t	duckspeed;
	int32_t	waterspeed;
	int32_t	accel;
	int32_t	stopspeed;
#endif

	array<float, 4>	blend;	// rgba full screen effect

	float	fov;	// horizontal field of view

	refdef_flags	rdflags;        // refdef flags

	array<player_stat, MAX_STATS>	stats;       // fast status bar updates
};

// client data, only stored in entities between 1 and maxclients inclusive
struct client
{
	// shared between client and server

	player_state	ps;
	int				ping;
	int				clientNum;

	gclient			g;
};
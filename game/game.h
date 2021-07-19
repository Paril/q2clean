#pragma once

#include "lib/types.h"

import string;
import math.vector;

// Contains some constants and structures that are integral to game states.

// dmflags.value flags
enum dm_flags : uint32_t
{
	DF_NO_HEALTH = 1 << 0,
	DF_NO_ITEMS = 1 << 1,
	DF_WEAPONS_STAY = 1 << 2,
	DF_NO_FALLING = 1 << 3,
	DF_INSTANT_ITEMS = 1 << 4,
	DF_SAME_LEVEL = 1 << 5,
	DF_SKINTEAMS = 1 << 6,
	DF_MODELTEAMS = 1 << 7,
	DF_NO_FRIENDLY_FIRE = 1 << 8,
	DF_SPAWN_FARTHEST = 1 << 9,
	DF_FORCE_RESPAWN = 1 << 10,
	DF_NO_ARMOR = 1 << 11,
	DF_ALLOW_EXIT = 1 << 12,
	DF_INFINITE_AMMO = 1 << 13,
	DF_QUAD_DROP = 1 << 14,
	DF_FIXED_FOV = 1 << 15,		// unused; this is outdated and unnecessary

// RAFAEL
	DF_QUADFIRE_DROP = 1 << 16,
// RAFAEL

//ROGUE
	DF_NO_MINES = 1 << 17,
	DF_NO_STACK_DOUBLE = 1 << 18,
	DF_NO_NUKES = 1 << 19,
	DF_NO_SPHERES = 1 << 20
//ROGUE

#ifdef CTF
	, DF_CTF_FORCEJOIN = 1 << 21,
	DF_ARMOR_PROTECT = 1 << 22,
	DF_CTF_NO_TECH = 1 << 23
#endif
};

// weapon parameters
constexpr int32_t DEFAULT_BULLET_HSPREAD	= 300;
constexpr int32_t DEFAULT_BULLET_VSPREAD	= 500;
constexpr int32_t DEFAULT_SHOTGUN_HSPREAD	= 1000;
constexpr int32_t DEFAULT_SHOTGUN_VSPREAD	= 500;
constexpr int32_t DEFAULT_SHOTGUN_COUNT		= 12;
constexpr int32_t DEFAULT_SSHOTGUN_COUNT	= 20;

import protocol;

extern sound_index snd_fry;
extern model_index sm_meat_index;

// global cvars
#ifdef SINGLE_PLAYER
extern cvarref	deathmatch;
extern cvarref	coop;
extern cvarref	skill;
#endif
extern cvarref	dmflags;
extern cvarref	fraglimit;
extern cvarref	timelimit;
#ifdef CTF
extern cvarref	ctf;
extern cvarref	capturelimit;
extern cvarref	instantweap;
#endif
extern cvarref	password;
extern cvarref	spectator_password;
extern cvarref	needpass;
extern cvarref	maxspectators;
extern cvarref	g_select_empty;
extern cvarref	dedicated;

extern cvarref	filterban;

extern cvarref	sv_maxvelocity;
extern cvarref	sv_gravity;

extern cvarref	sv_rollspeed;
extern cvarref	sv_rollangle;

extern cvarref	run_pitch;
extern cvarref	run_roll;
extern cvarref	bob_up;
extern cvarref	bob_pitch;
extern cvarref	bob_roll;

extern cvarref	sv_cheats;

extern cvarref	flood_msgs;
extern cvarref	flood_persecond;
extern cvarref	flood_waitdelay;

extern cvarref	sv_maplist;

extern cvarref	sv_features;

// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay.
// if you add more, be sure to edit spawn_fields!
struct spawn_temp
{
	string	classname;
	
	int32_t	lip;
	int32_t	distance;
	int32_t	height;
	string	noise;
	float	pausetime;
	string	item;
	string	gravity;
	
	float	minyaw;
	float	maxyaw;
	float	minpitch;
	float	maxpitch;

	// world vars
	string	sky;
	float	skyrotate;
	vector	skyaxis;
	string	nextmap;
};

extern spawn_temp st;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
struct level_locals
{
	gtime	framenum;
	float	time;

	string	level_name;  // the descriptive name (Outer Base, etc)
	string	mapname;     // the server name (base1, etc)
	string	nextmap;     // go here when fraglimit is hit

	// intermission state
	gtime	intermission_framenum;  // time the intermission was started
	string	changemap;
	bool	exitintermission;
	vector	intermission_origin;
	vector	intermission_angle;

	image_index	pic_health;

#ifdef SINGLE_PLAYER
	entityref	sight_client;	// changed once each frame for coop games

	entityref	sight_entity;
	gtime		sight_entity_framenum;
	entityref	sound_entity;
	gtime		sound_entity_framenum;
	entityref	sound2_entity;
	gtime		sound2_entity_framenum;

	int32_t	total_secrets;
	int32_t	found_secrets;

	int32_t	total_goals;
	int32_t	found_goals;

	int32_t	total_monsters;
	int32_t	killed_monsters;

	int32_t	power_cubes;        // ugly necessity for coop

#ifdef GROUND_ZERO
	entityref	disguise_violator;
	gtime		disguise_violation_framenum;
#endif
#endif

	entityref	current_entity;	// entity running from G_RunFrame

	int32_t	body_que;           // dead bodies
};

extern level_locals level;

/*
============
InitGame

This will be called when the game is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame();

/*
============
ShutdownGame

Called when the game DLL is being shut down.
============
*/
void ShutdownGame();

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel();

/*
================
RunFrame

Advances the world
================
*/
void RunFrame();
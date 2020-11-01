#pragma once

#include "types.h"
#include "dynarray.h"

// impl passed from engine.
extern "C" struct game_import_impl;

#include "print_level.h"
#include "sound_channel.h"
#include "sound_attn.h"

// max bytes in a single message packet
constexpr size_t MESSAGE_LIMIT = 1400;

// max size of a Quake path
constexpr size_t MAX_QPATH = 64;

#include "config_string.h"
#include "multicast_destination.h"
#include "box_edicts_area.h"
#include "surface_flags.h"
#include "surface.h"
#include "cvar.h"

//
// muzzle flashes / player effects
//
enum muzzleflash : uint8_t
{
	MZ_BLASTER,
	MZ_MACHINEGUN,
	MZ_SHOTGUN,
	MZ_CHAINGUN1,
	MZ_CHAINGUN2,
	MZ_CHAINGUN3,
	MZ_RAILGUN,
	MZ_ROCKET,
	MZ_GRENADE,
	MZ_LOGIN,
	MZ_LOGOUT,
	MZ_RESPAWN,
	MZ_BFG,
	MZ_SSHOTGUN,
	MZ_HYPERBLASTER,
	MZ_ITEMRESPAWN,
// RAFAEL
	MZ_IONRIPPER,
	MZ_BLUEHYPERBLASTER,
	MZ_PHALANX,
// RAFAEL

//ROGUE
	MZ_ETF_RIFLE	= 30,
	MZ_UNUSED,
	MZ_SHOTGUN2,
	MZ_HEATBEAM,
	MZ_BLASTER2,
	MZ_TRACKER,
	MZ_NUKE1,
	MZ_NUKE2,
	MZ_NUKE4,
	MZ_NUKE8,
//ROGUE

	MZ_SILENCED		= 128,	// bit flag ORed with one of the above numbers,
	MZ_NONE			= 0
};

MAKE_ENUM_BITWISE(muzzleflash)

//
// monster muzzle flashes
//
enum monster_muzzleflash : uint8_t
{
	MZ2_TANK_BLASTER_1	= 1,
	MZ2_TANK_BLASTER_2,
	MZ2_TANK_BLASTER_3,
	MZ2_TANK_MACHINEGUN_1,
	MZ2_TANK_MACHINEGUN_2,
	MZ2_TANK_MACHINEGUN_3,
	MZ2_TANK_MACHINEGUN_4,
	MZ2_TANK_MACHINEGUN_5,
	MZ2_TANK_MACHINEGUN_6,
	MZ2_TANK_MACHINEGUN_7,
	MZ2_TANK_MACHINEGUN_8,
	MZ2_TANK_MACHINEGUN_9,
	MZ2_TANK_MACHINEGUN_10,
	MZ2_TANK_MACHINEGUN_11,
	MZ2_TANK_MACHINEGUN_12,
	MZ2_TANK_MACHINEGUN_13,
	MZ2_TANK_MACHINEGUN_14,
	MZ2_TANK_MACHINEGUN_15,
	MZ2_TANK_MACHINEGUN_16,
	MZ2_TANK_MACHINEGUN_17,
	MZ2_TANK_MACHINEGUN_18,
	MZ2_TANK_MACHINEGUN_19,
	MZ2_TANK_ROCKET_1,
	MZ2_TANK_ROCKET_2,
	MZ2_TANK_ROCKET_3,

	MZ2_INFANTRY_MACHINEGUN_1,
	MZ2_INFANTRY_MACHINEGUN_2,
	MZ2_INFANTRY_MACHINEGUN_3,
	MZ2_INFANTRY_MACHINEGUN_4,
	MZ2_INFANTRY_MACHINEGUN_5,
	MZ2_INFANTRY_MACHINEGUN_6,
	MZ2_INFANTRY_MACHINEGUN_7,
	MZ2_INFANTRY_MACHINEGUN_8,
	MZ2_INFANTRY_MACHINEGUN_9,
	MZ2_INFANTRY_MACHINEGUN_10,
	MZ2_INFANTRY_MACHINEGUN_11,
	MZ2_INFANTRY_MACHINEGUN_12,
	MZ2_INFANTRY_MACHINEGUN_13,
	
	MZ2_SOLDIER_BLASTER_1,
	MZ2_SOLDIER_BLASTER_2,
	MZ2_SOLDIER_SHOTGUN_1,
	MZ2_SOLDIER_SHOTGUN_2,
	MZ2_SOLDIER_MACHINEGUN_1,
	MZ2_SOLDIER_MACHINEGUN_2,
	
	MZ2_GUNNER_MACHINEGUN_1,
	MZ2_GUNNER_MACHINEGUN_2,
	MZ2_GUNNER_MACHINEGUN_3,
	MZ2_GUNNER_MACHINEGUN_4,
	MZ2_GUNNER_MACHINEGUN_5,
	MZ2_GUNNER_MACHINEGUN_6,
	MZ2_GUNNER_MACHINEGUN_7,
	MZ2_GUNNER_MACHINEGUN_8,
	MZ2_GUNNER_GRENADE_1,
	MZ2_GUNNER_GRENADE_2,
	MZ2_GUNNER_GRENADE_3,
	MZ2_GUNNER_GRENADE_4,

	MZ2_CHICK_ROCKET_1,
	
	MZ2_FLYER_BLASTER_1,
	MZ2_FLYER_BLASTER_2,
	
	MZ2_MEDIC_BLASTER_1,
	
	MZ2_GLADIATOR_RAILGUN_1,
	
	MZ2_HOVER_BLASTER_1,
	
	MZ2_ACTOR_MACHINEGUN_1,
	
	MZ2_SUPERTANK_MACHINEGUN_1,
	MZ2_SUPERTANK_MACHINEGUN_2,
	MZ2_SUPERTANK_MACHINEGUN_3,
	MZ2_SUPERTANK_MACHINEGUN_4,
	MZ2_SUPERTANK_MACHINEGUN_5,
	MZ2_SUPERTANK_MACHINEGUN_6,
	MZ2_SUPERTANK_ROCKET_1,
	MZ2_SUPERTANK_ROCKET_2,
	MZ2_SUPERTANK_ROCKET_3,
	
	MZ2_BOSS2_MACHINEGUN_L1,
	MZ2_BOSS2_MACHINEGUN_L2,
	MZ2_BOSS2_MACHINEGUN_L3,
	MZ2_BOSS2_MACHINEGUN_L4,
	MZ2_BOSS2_MACHINEGUN_L5,
	MZ2_BOSS2_ROCKET_1,
	MZ2_BOSS2_ROCKET_2,
	MZ2_BOSS2_ROCKET_3,
	MZ2_BOSS2_ROCKET_4,
	
	MZ2_FLOAT_BLASTER_1,

	MZ2_SOLDIER_BLASTER_3,
	MZ2_SOLDIER_SHOTGUN_3,
	MZ2_SOLDIER_MACHINEGUN_3,
	MZ2_SOLDIER_BLASTER_4,
	MZ2_SOLDIER_SHOTGUN_4,
	MZ2_SOLDIER_MACHINEGUN_4,
	MZ2_SOLDIER_BLASTER_5,
	MZ2_SOLDIER_SHOTGUN_5,
	MZ2_SOLDIER_MACHINEGUN_5,
	MZ2_SOLDIER_BLASTER_6,
	MZ2_SOLDIER_SHOTGUN_6,
	MZ2_SOLDIER_MACHINEGUN_6,
	MZ2_SOLDIER_BLASTER_7,
	MZ2_SOLDIER_SHOTGUN_7,
	MZ2_SOLDIER_MACHINEGUN_7,
	MZ2_SOLDIER_BLASTER_8,
	MZ2_SOLDIER_SHOTGUN_8,
	MZ2_SOLDIER_MACHINEGUN_8,

// Xian
	MZ2_MAKRON_BFG,
	MZ2_MAKRON_BLASTER_1,
	MZ2_MAKRON_BLASTER_2,
	MZ2_MAKRON_BLASTER_3,
	MZ2_MAKRON_BLASTER_4,
	MZ2_MAKRON_BLASTER_5,
	MZ2_MAKRON_BLASTER_6,
	MZ2_MAKRON_BLASTER_7,
	MZ2_MAKRON_BLASTER_8,
	MZ2_MAKRON_BLASTER_9,
	MZ2_MAKRON_BLASTER_10,
	MZ2_MAKRON_BLASTER_11,
	MZ2_MAKRON_BLASTER_12,
	MZ2_MAKRON_BLASTER_13,
	MZ2_MAKRON_BLASTER_14,
	MZ2_MAKRON_BLASTER_15,
	MZ2_MAKRON_BLASTER_16,
	MZ2_MAKRON_BLASTER_17,
	MZ2_MAKRON_RAILGUN_1,
	MZ2_JORG_MACHINEGUN_L1,
	MZ2_JORG_MACHINEGUN_L2,
	MZ2_JORG_MACHINEGUN_L3,
	MZ2_JORG_MACHINEGUN_L4,
	MZ2_JORG_MACHINEGUN_L5,
	MZ2_JORG_MACHINEGUN_L6,
	MZ2_JORG_MACHINEGUN_R1,
	MZ2_JORG_MACHINEGUN_R2,
	MZ2_JORG_MACHINEGUN_R3,
	MZ2_JORG_MACHINEGUN_R4,
	MZ2_JORG_MACHINEGUN_R5,
	MZ2_JORG_MACHINEGUN_R6,
	MZ2_JORG_BFG_1,
	MZ2_BOSS2_MACHINEGUN_R1,
	MZ2_BOSS2_MACHINEGUN_R2,
	MZ2_BOSS2_MACHINEGUN_R3,
	MZ2_BOSS2_MACHINEGUN_R4,
	MZ2_BOSS2_MACHINEGUN_R5,
// Xian

//ROGUE
	MZ2_CARRIER_MACHINEGUN_L1,
	MZ2_CARRIER_MACHINEGUN_R1,
	MZ2_CARRIER_GRENADE,
	MZ2_TURRET_MACHINEGUN,
	MZ2_TURRET_ROCKET,
	MZ2_TURRET_BLASTER,
	MZ2_STALKER_BLASTER,
	MZ2_DAEDALUS_BLASTER,
	MZ2_MEDIC_BLASTER_2,
	MZ2_CARRIER_RAILGUN,
	MZ2_WIDOW_DISRUPTOR,
	MZ2_WIDOW_BLASTER,
	MZ2_WIDOW_RAIL,
	MZ2_WIDOW_PLASMABEAM,			// PMM - not used
	MZ2_CARRIER_MACHINEGUN_L2,
	MZ2_CARRIER_MACHINEGUN_R2,
	MZ2_WIDOW_RAIL_LEFT,
	MZ2_WIDOW_RAIL_RIGHT,
	MZ2_WIDOW_BLASTER_SWEEP1,
	MZ2_WIDOW_BLASTER_SWEEP2,
	MZ2_WIDOW_BLASTER_SWEEP3,
	MZ2_WIDOW_BLASTER_SWEEP4,
	MZ2_WIDOW_BLASTER_SWEEP5,
	MZ2_WIDOW_BLASTER_SWEEP6,
	MZ2_WIDOW_BLASTER_SWEEP7,
	MZ2_WIDOW_BLASTER_SWEEP8,
	MZ2_WIDOW_BLASTER_SWEEP9,
	MZ2_WIDOW_BLASTER_100,
	MZ2_WIDOW_BLASTER_90,
	MZ2_WIDOW_BLASTER_80,
	MZ2_WIDOW_BLASTER_70,
	MZ2_WIDOW_BLASTER_60,
	MZ2_WIDOW_BLASTER_50,
	MZ2_WIDOW_BLASTER_40,
	MZ2_WIDOW_BLASTER_30,
	MZ2_WIDOW_BLASTER_20,
	MZ2_WIDOW_BLASTER_10,
	MZ2_WIDOW_BLASTER_0,
	MZ2_WIDOW_BLASTER_10L,
	MZ2_WIDOW_BLASTER_20L,
	MZ2_WIDOW_BLASTER_30L,
	MZ2_WIDOW_BLASTER_40L,
	MZ2_WIDOW_BLASTER_50L,
	MZ2_WIDOW_BLASTER_60L,
	MZ2_WIDOW_BLASTER_70L,
	MZ2_WIDOW_RUN_1,
	MZ2_WIDOW_RUN_2,
	MZ2_WIDOW_RUN_3,
	MZ2_WIDOW_RUN_4,
	MZ2_WIDOW_RUN_5,
	MZ2_WIDOW_RUN_6,
	MZ2_WIDOW_RUN_7,
	MZ2_WIDOW_RUN_8,
	MZ2_CARRIER_ROCKET_1,
	MZ2_CARRIER_ROCKET_2,
	MZ2_CARRIER_ROCKET_3,
	MZ2_CARRIER_ROCKET_4,
	MZ2_WIDOW2_BEAMER_1,
	MZ2_WIDOW2_BEAMER_2,
	MZ2_WIDOW2_BEAMER_3,
	MZ2_WIDOW2_BEAMER_4,
	MZ2_WIDOW2_BEAMER_5,
	MZ2_WIDOW2_BEAM_SWEEP_1,
	MZ2_WIDOW2_BEAM_SWEEP_2,
	MZ2_WIDOW2_BEAM_SWEEP_3,
	MZ2_WIDOW2_BEAM_SWEEP_4,
	MZ2_WIDOW2_BEAM_SWEEP_5,
	MZ2_WIDOW2_BEAM_SWEEP_6,
	MZ2_WIDOW2_BEAM_SWEEP_7,
	MZ2_WIDOW2_BEAM_SWEEP_8,
	MZ2_WIDOW2_BEAM_SWEEP_9,
	MZ2_WIDOW2_BEAM_SWEEP_10,
	MZ2_WIDOW2_BEAM_SWEEP_11
// ROGUE
};

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
enum temp_event : uint8_t
{
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,	// used as '22' in a map, so DON'T RENUMBER!!!
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
//ROGUE
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE,
//ROGUE

	TE_NUM_ENTITIES
};

// color for TE_SPLASH
enum splash_type : uint8_t
{
	SPLASH_UNKNOWN,
	SPLASH_SPARKS,
	SPLASH_BLUE_WATER,
	SPLASH_BROWN_WATER,
	SPLASH_SLIME,
	SPLASH_LAVA,
	SPLASH_BLOOD
};

// messages that can be sent to players
enum svc_ops : int32_t
{
	svc_bad,

	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	svc_sound = 9,			// <see code>
	svc_print,			// [byte] id [string] null terminated string
	svc_stufftext,			// [string] stuffed into client's console buffer, should be \n terminated
	svc_configstring = 13,		// [short] [string]
	svc_centerprint = 15		// [string] to put in center of the screen
};

#include "trace.h"
#include "pmove_state.h"
#include "usercmd.h"
#include "dynarray.h"

constexpr size_t MAX_TOUCH = 32;

extern "C" struct pmove_t
{
	// state (in / out)
	pmove_state	s;

	// command (in)
	usercmd	cmd;

	// if s has been changed outside pmove
	qboolean	snapinitial;

	// results (out)

	int32_t						numtouch;
	array<entityref, MAX_TOUCH>	touchents;

	vector	viewangles;         // clamped
	float	viewheight;

	vector	mins, maxs;         // bounding box size

	entityref		groundentity;
	content_flags	watertype;
	int32_t			waterlevel;

	// callbacks to test the world
	trace			(*trace)(const vector &start, const vector &mins, const vector &maxs, const vector &end);
	content_flags	(*pointcontents)(const vector &point);
};

struct game_import
{
	// internal function to assign the implementation
	void set_impl(game_import_impl *implptr);

	// whether we can call game functions yet or not
	bool is_ready();

	// memory functions

	template<typename T>
	[[nodiscard]] inline T *TagMalloc(const uint32_t &count, const uint32_t &tag)
	{
		return (T *)TagMalloc(sizeof(T) * count, tag);
	}

	[[nodiscard]] void *TagMalloc(uint32_t size, uint32_t tag);

	void TagFree(void *block);

	void FreeTags(uint32_t tag);

	// chat functions

	// broadcast print; prints to all connected clients + console
	void bprintf(print_level printlevel, stringlit fmt, ...);

	// debug print; prints only to console or listen server host
	void dprintf(stringlit fmt, ...);

	// client print; prints to one client
	void cprintf(const entity &ent, print_level printlevel, stringlit fmt, ...);

	// center print; prints on center of screen to client
	void centerprintf(const entity &ent, stringlit fmt, ...);

	// sounds

	// fetch a sound index from the specified sound file
	sound_index soundindex(const stringref &name);

	// sound; play soundindex on specified entity on channel at specified volume/attn with a time offset of timeofs
	void sound(const entity &ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float timeofs);
	
	// sound; play soundindex at specified origin (from specified entity) on channel at specified volume/attn with a time offset of timeofs
	void positioned_sound(vector origin, entityref ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float timeofs);

	// models

	// fetch a model index from the specified sound file
	model_index modelindex(const stringref &name);
	
	// set model to specified string value; use this for bmodels, also sets mins/maxs
	void setmodel(entity &ent, const stringref &name);

	// images

	// fetch a model index from the specified sound file
	image_index imageindex(const stringref &name);

	// entities
	
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void linkentity(entity &ent);
	// call before removing an interactive edict
	void unlinkentity(entity &ent);
	// return entities within the specified box
	dynarray<entityref> BoxEdicts(vector mins, vector maxs, box_edicts_area areatype, uint32_t allocate = 16);
	// player movement code common with client prediction
	void Pmove(pmove_t &pmove);

	// collision
	
	// perform a line trace
	[[nodiscard]] trace traceline(vector start, vector end, entityref passent, content_flags contentmask);
	// perform a box trace
	[[nodiscard]] trace trace(vector start, vector mins, vector maxs, vector end, entityref passent, content_flags contentmask);
	// fetch the brush contents at the specified point
	[[nodiscard]] content_flags pointcontents(vector point);
	// check whether the two vectors are in the same PVS
	[[nodiscard]] bool inPVS(vector p1, vector p2);
	// check whether the two vectors are in the same PHS
	[[nodiscard]] bool inPHS(vector p1, vector p2);
	// set the state of the specified area portal
	void SetAreaPortalState(int32_t portalnum, bool open);
	// check whether the two area indices are connected to each other
	[[nodiscard]] bool AreasConnected(area_index area1, area_index area2);

	// network

	void WriteChar(int8_t c);
	void WriteByte(uint8_t c);
	void WriteShort(int16_t c);
	void WriteEntity(const entity &e);
	void WriteLong(int32_t c);
	void WriteFloat(float f);
	void WriteString(stringlit s);
	inline void WriteString(const stringref &s)
	{
		WriteString(s.ptr());
	}
	void WritePosition(vector pos);
	void WriteDir(vector pos);
	void WriteAngle(float f);

	// send the currently-buffered message to all clients within
	// the specified destination
	void multicast(vector origin, multicast_destination to);
	
	// send the currently-buffered message to the specified
	// client.
	void unicast(const entity &ent, bool reliable);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void configstring(config_string num, const stringref &string);

	// parameters for ClientCommand and ServerCommand

	// return arg count
	[[nodiscard]] size_t argc();
	// get specific argument
	[[nodiscard]] stringlit argv(size_t n);
	// get concatenation of arguments >= 1
	[[nodiscard]] stringlit args();

	// cvars
	
	cvar &cvar_forceset(stringlit var_name, const stringref &value);
	cvar &cvar_set(stringlit var_name, const stringref &value);
	cvar &cvar(stringlit var_name, const stringref &value, cvar_flags flags);

	// misc

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void AddCommandString(const stringref &text);

	void DebugGraph(float value, int color);

	// drop to console, error
	[[noreturn]] void error(stringlit fmt, ...);
};

extern game_import gi;
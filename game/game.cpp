#include "entity.h"
#include "player.h"
#include "phys.h"
#include "spawn.h"
#ifdef SINGLE_PLAYER
#include "ai.h"
#include "monster.h"
#endif

#include "lib/gi.h"
#include "game.h"
#include "util.h"
#include "hud.h"
#include "lib/string/format.h"
#include "view.h"
#include "target.h"

constexpr stringlit GAMEVERSION = "clean";

spawn_temp st;
level_locals level;

game_locals game;

// global cvars
#ifdef SINGLE_PLAYER
cvarref	deathmatch;
cvarref	coop;
cvarref	skill;
#endif
cvarref	dmflags;
cvarref	fraglimit;
cvarref	timelimit;
#ifdef CTF
cvarref	ctf;
cvarref	capturelimit;
cvarref	instantweap;
#endif
cvarref	password;
cvarref	spectator_password;
cvarref	needpass;
cvarref	maxspectators;
cvarref	g_select_empty;
cvarref	dedicated;

cvarref	filterban;

cvarref	sv_maxvelocity;
cvarref	sv_gravity;

cvarref	sv_rollspeed;
cvarref	sv_rollangle;

cvarref	run_pitch;
cvarref	run_roll;
cvarref	bob_up;
cvarref	bob_pitch;
cvarref	bob_roll;

cvarref	sv_cheats;

cvarref	flood_msgs;
cvarref	flood_persecond;
cvarref	flood_waitdelay;

cvarref	sv_maplist;

cvarref	sv_features;

model_index sm_meat_index;
sound_index snd_fry;

client *clients;

void InitGame()
{
	gi.dprintfmt("===== {} =====\n", __func__);
	
	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar("sv_rollspeed", "200", CVAR_NONE);
	
	sv_rollangle = gi.cvar("sv_rollangle", "2", CVAR_NONE);
	sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", CVAR_NONE);
	sv_gravity = gi.cvar("sv_gravity", "800", CVAR_NONE);
	
	// noset vars
	dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);
	
	// latched vars
	sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);
	
	cvarref maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
#ifdef SINGLE_PLAYER
	deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
	coop = gi.cvar("coop", "0", CVAR_LATCH);
	skill = gi.cvar("skill", "1", CVAR_LATCH);
#else
	gi.cvar("deathmatch", "1", CVAR_NOSET);
	gi.cvar_forceset("deathmatch", "1");
	gi.cvar_forceset("coop", "0");
#endif
	gi.cvarfmt("maxentities", CVAR_NOSET, "{}", MAX_EDICTS);
	
	// change anytime vars
	dmflags = gi.cvar("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
#ifdef CTF
	capturelimit = gi.cvar("capturelimit", "0", CVAR_SERVERINFO);
	instantweap = gi.cvar("instantweap", "0", CVAR_SERVERINFO);
#endif
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar("filterban", "1", CVAR_NONE);
	
	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);
	
	run_pitch = gi.cvar("run_pitch", "0.002", CVAR_NONE);
	run_roll = gi.cvar("run_roll", "0.005", CVAR_NONE);
	bob_up  = gi.cvar("bob_up", "0.005", CVAR_NONE);
	bob_pitch = gi.cvar("bob_pitch", "0.002", CVAR_NONE);
	bob_roll = gi.cvar("bob_roll", "0.002", CVAR_NONE);
	
	// flood control
	flood_msgs = gi.cvar("flood_msgs", "4", CVAR_NONE);
	flood_persecond = gi.cvar("flood_persecond", "4", CVAR_NONE);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", CVAR_NONE);
	
	// dm map list
	sv_maplist = gi.cvar("sv_maplist", "", CVAR_NONE);
	
	// obtain server features
	sv_features = gi.cvar("sv_features", "", CVAR_NONE);
	
	// export our own features
	gi.cvar_forcesetfmt("g_features", "{}", (int32_t) G_FEATURES);

	game.maxclients = (uint32_t) maxclients;

	num_entities = game.maxclients + 1;

	// allocate client storage; this is used by main.cpp as well
	clients = gi.TagMalloc<client>(game.maxclients, TAG_GAME);
	
	// initialize the client entities, so the ptrs are valid
	for (size_t i = 0; i < game.maxclients; i++)
	{
		::new(&clients[i]) client;
		itoe(i + 1).__init();
	}

	InitItems();

#ifdef CTF
	CTFInit();
#endif
}

void ShutdownGame()
{
	gi.dprintfmt("===== {} =====\n", __func__);
}

/*
=================
ClientEndServerFrames
=================
*/
static void ClientEndServerFrames()
{
    // calc the player views now that all pushing
    // and damage has been added
	for (entity &ent : entity_range(1, game.maxclients))
		if (ent.inuse)
			ClientEndServerFrame(ent);
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
static entity &CreateTargetChangeLevel(string new_map)
{
	entity &ent = G_Spawn();
	ent.type = ET_TARGET_CHANGELEVEL;
	ent.map = level.nextmap = new_map;
	return ent;
}

static entity &FindTargetChangeLevel()
{
	// stay on same level flag
	if (dmflags & DF_SAME_LEVEL)
		return CreateTargetChangeLevel(level.mapname);

	// see if it's in the map list
	if (sv_maplist)
	{
		stringlit ml = (stringlit)sv_maplist;
		size_t s_offset = 0;
		string t = strtok(ml, s_offset), f = t;

		while (t)
		{
			if (striequals(t, level.mapname))
			{
				// it's in the list, go to the next one
				t = strtok(ml, s_offset);
				if (t)
					return CreateTargetChangeLevel(t);
				// end of list, go to first one
				else if (!f)
					return CreateTargetChangeLevel(f);

				// there isn't a first one, same level
				return CreateTargetChangeLevel(level.mapname);
			}

			t = strtok(ml, s_offset);
		}
	}

	if (level.nextmap) // go to a specific map
		return CreateTargetChangeLevel(level.nextmap);

	// search for a changelevel
	entityref ent = G_FindEquals<&entity::type>(world, ET_TARGET_CHANGELEVEL);

	if (ent.has_value())
		return ent;

	// the map designer didn't include a changelevel,
	// so create a fake ent that goes back to the same level
	return CreateTargetChangeLevel(level.mapname);
}

void EndDMLevel()
{
	BeginIntermission(FindTargetChangeLevel());
}

/*
=================
CheckNeedPass
=================
*/
static void CheckNeedPass()
{
	// if password or spectator_password has changed, update needpass
	// as needed
	if (password.modified || spectator_password.modified)
	{
		password.modified = spectator_password.modified = false;
		int need = 0;

		if (password && password != "none")
			need |= 1;
		if (spectator_password && spectator_password != "none")
			need |= 2;

		gi.cvar_setfmt("needpass", "{}", need);
	}
}

/*
=================
CheckDMRules
=================
*/
static void CheckDMRules()
{
	if (level.intermission_time != gtime::zero())
		return;

#ifdef SINGLE_PLAYER
	if (!deathmatch)
		return;
#endif

#ifdef CTF
	if (ctf.intVal && CTFCheckRules())
	{
		EndDMLevel ();
		return;
	}
#endif

	if (timelimit)
	{
		if (level.time >= minutes((int32_t) timelimit))
		{
			gi.bprint(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if (fraglimit)
	{
		for (entity &cl : entity_range(1, game.maxclients))
		{
			if (!cl.inuse)
				continue;

			if (cl.client.resp.score >= fraglimit)
			{
				gi.bprint(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel();
				return;
			}
		}
	}
}

/*
=============
ExitLevel
=============
*/
static void ExitLevel()
{
	level.exitintermission = 0;
	level.intermission_time = gtime::zero();
	
	gi.AddCommandStringFmt("gamemap \"{}\"\n", level.changemap);
	level.changemap = nullptr;
	
	ClientEndServerFrames();
	
	// clear some things before going to next level
	for (entity &ent : entity_range(1, game.maxclients))
	{
		if (!ent.inuse)
			continue;
	
		if (ent.health > ent.max_health)
			ent.health = ent.max_health;
	}
}

void RunFrame()
{
	level.time += framerate_ms;

	// exit intermissions
	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}
#ifdef SINGLE_PLAYER

	// choose a client for monsters to target this frame
	AI_SetSightClient();
#endif
	
	//
	// treat each object in turn
	// even the world gets a chance to think.
	// this is done in a loop since the last entity ID
	// can move during a think.
	//
	for (uint32_t i = 0; i < num_entities; i++)
	{
		entity &ent = itoe(i);

		if (!ent.inuse)
			continue;
		
		level.current_entity = ent;
		
		ent.old_origin = ent.origin;
		
		// if the ground entity moved, make sure we are still on it
		if (ent.groundentity.has_value() && (ent.groundentity->linkcount != ent.groundentity_linkcount))
#ifdef SINGLE_PLAYER
		{
#endif
			ent.groundentity = null_entity;
#ifdef SINGLE_PLAYER
			if (!(ent.flags & (FL_SWIM | FL_FLY)) && (ent.svflags & SVF_MONSTER))
				M_CheckGround(ent);
		}
#endif
	
		if (ent.is_client)
			ClientBeginServerFrame(ent);
	
		G_RunEntity(ent);
	}
	
	// see if it is time to end a deathmatch
	CheckDMRules();
	
	// see if needpass needs updated
	CheckNeedPass();
	
	// build the playerstate_t structures for all players
	ClientEndServerFrames();
}
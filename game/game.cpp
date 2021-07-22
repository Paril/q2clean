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

void InitGame()
{
	gi.dprintf("===== %s =====\n", __func__);
	
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
	cvarref maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);
	
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
	gi.cvar_forceset("g_features", va("%i", G_FEATURES));

	game.maxclients = (uint32_t)maxclients;

	num_entities = game.maxclients + 1;

	// s.number and client must always be assigned
	for (auto &e : entity_range(1, game.maxclients))
	{
		e.client = gi.TagMalloc<client>(1, TAG_GAME);
		::new(e.client) client;
	}
	
	InitItems();

#ifdef CTF
	CTFInit();
#endif
}

void ShutdownGame()
{
	gi.dprintf("===== %s =====\n", __func__);
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
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		entity &ent = itoe(1 + i);
		
		if (ent.inuse && ent.is_client())
			ClientEndServerFrame(ent);
	}
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
	level.nextmap = new_map;
	ent.map = level.nextmap;
	return ent;
}

void EndDMLevel()
{
	// stay on same level flag
	if ((dm_flags)dmflags & DF_SAME_LEVEL)
	{
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	// see if it's in the map list
	if (sv_maplist)
	{
		stringlit ml = (stringlit)sv_maplist;
		string f;
		size_t s_offset = 0;
		string t = strtok(ml, s_offset);

		while (t)
		{
			if (striequals(t, level.mapname))
			{
				// it's in the list, go to the next one
				t = strtok(ml, s_offset);
				if (!t)
				{ // end of list, go to first one
					if (!f) // there isn't a first one, same level
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
					else
						BeginIntermission(CreateTargetChangeLevel(f));
				}
				else
					BeginIntermission(CreateTargetChangeLevel(t));

				return;
			}
			if (!f)
				f = t;
			t = strtok(ml, s_offset);
		}
	}

	if (level.nextmap) // go to a specific map
		BeginIntermission(CreateTargetChangeLevel(level.nextmap));
	else
	{  // search for a changelevel
		entityref ent = G_FindEquals<&entity::type>(world, ET_TARGET_CHANGELEVEL);
		if (!ent.has_value())
		{
			// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission(CreateTargetChangeLevel(level.mapname));
			return;
		}
		BeginIntermission(ent);
	}
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

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
static void CheckDMRules()
{
	if (level.intermission_framenum)
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

	if ((int32_t)timelimit)
	{
		if (level.time >= (int32_t)timelimit * 60)
		{
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if ((int32_t)fraglimit)
	{
		for (uint32_t i = 0; i < game.maxclients; i++)
		{
			entity &cl = itoe(i + 1);

			if (!cl.inuse)
				continue;

			if (cl.client->resp.score >= (int32_t)fraglimit)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
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
	level.intermission_framenum = 0;
	
	gi.AddCommandString(va("gamemap \"%s\"\n", level.changemap.ptr()));
	level.changemap = nullptr;
	
	ClientEndServerFrames();
	
	// clear some things before going to next level
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		entity &ent = itoe(1 + i);
	
		if (!ent.inuse)
			continue;
	
		if (ent.health > ent.max_health)
			ent.health = ent.max_health;
	}
}

void RunFrame()
{
	level.framenum++;
	level.time = level.framenum * FRAMETIME;

#ifdef SINGLE_PLAYER
	// choose a client for monsters to target this frame
	AI_SetSightClient();
#endif
	
	// exit intermissions
	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}
	
	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	for (uint32_t i = 0 ; i < num_entities; i++)
	{
		entity &ent = itoe(i);

		if (!ent.inuse)
			continue;
		
		level.current_entity = ent;
		
		ent.s.old_origin = ent.s.origin;
		
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
	
		if (i > 0 && i <= game.maxclients)
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
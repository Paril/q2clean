#include "lib/types.h"
#include "lib/gi.h"
#include "lib/entity.h"
#include "lib/info.h"
#include "game/player.h"
#include "game/game.h"
#include "game/cmds.h"
#include "game/svcmds.h"
#include "game/spawn.h"

static void WipeEntities()
{
	for (auto &e : entity_range(0, max_entities - 1))
		if (e.inuse)
			e.__free();
}

extern "C" struct game_export
{
	int	apiversion = 3;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void (*Init)() = ::InitGame;
	void (*Shutdown)() = ::ShutdownGame;

	// each new level entered will cause a call to SpawnEntities
	void (*SpawnEntities)(stringlit mapname, stringlit entstring, stringlit spawnpoint) = [](stringlit mapname, stringlit entstring, stringlit spawnpoint)
	{
		PreSpawnEntities();

		WipeEntities();

		::SpawnEntities(mapname, entstring, spawnpoint);
	};

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void (*WriteGame)(stringlit filename, qboolean autosave) = [](stringlit filename [[maybe_unused]], qboolean autosave [[maybe_unused]])
	{
	};
	void (*ReadGame)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
	};

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void (*WriteLevel)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
	};
	void (*ReadLevel)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
	};

	qboolean (*ClientConnect)(entity *ent, char *userinfo) = [](entity *ent, char *userinfo)
	{
		string ui(userinfo);
		
		const qboolean success = ::ClientConnect(*ent, ui);

		strcpy_s(userinfo, MAX_INFO_STRING, ui.ptr());

		return success;
	};
	void (*ClientBegin)(entity *ent) = [](entity *ent)
	{
		::ClientBegin(*ent);
	};
	void (*ClientUserinfoChanged)(entity *ent, char *userinfo) = [](entity *ent, char *userinfo)
	{
		::ClientUserinfoChanged(*ent, userinfo);
	};
	void (*ClientDisconnect)(entity *ent) = [](entity *ent)
	{
		::ClientDisconnect(*ent);
	};
	void (*ClientCommand)(entity *ent) = [](entity *ent)
	{
		::ClientCommand(*ent);
	};
	void (*ClientThink)(entity *ent, usercmd *cmd) = [](entity *ent, usercmd *cmd)
	{
		::ClientThink(*ent, *cmd);
	};

	void (*RunFrame)() = []()
	{
		::RunFrame();
	};

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void (*ServerCommand)() = []()
	{
		::ServerCommand();
	};

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	//
	// The size will be fixed when ge->Init() is called
	entity		*edicts;
	uint32_t	edict_size = sizeof(entity);
	uint32_t	num_edicts = 1;	// current number, <= max_edicts
	uint32_t	max_edicts = MAX_EDICTS;
};

static game_export ge;
entityref world;

void entity::__init()
{
	::client *cl = client;
	new(this) entity();
	this->s.number = this - ge.edicts;
	this->client = cl;
}

void entity::__free()
{
	::client *cl = client;
	this->~entity();
	memset(this, 0, sizeof(*this));
	this->s.number = this - ge.edicts;
	this->client = cl;
}

entity &itoe(size_t index)
{
	return ge.edicts[index];
}

entity_range_iterable::entity_range_iterable(size_t first, size_t last) :
	first(&itoe(first)),
	last(&itoe(last + 1))
{
}

entity *entity_range_iterable::begin()
{
	return first;
}

entity *entity_range_iterable::end()
{
	return last;
}

entity_range_iterable entity_range(size_t start, size_t end)
{
	if (end < start)
		std::swap(start, end);

	return entity_range_iterable(start, end);
}

uint32_t &num_entities = ge.num_edicts;
const uint32_t &max_entities = ge.max_edicts;

static inline void PreWriteGame(bool autosave [[maybe_unused]])
{
#ifdef SINGLE_PLAYER
	if (!autosave)
		SaveClientData();

	game.autosaved = autosave;
#endif
}

static inline void PostWriteGame()
{
#ifdef SINGLE_PLAYER
	game.autosaved = false;
#endif
}

static inline void PostReadLevel(uint32_t num_edicts [[maybe_unused]])
{
#ifdef SINGLE_PLAYER
	globals.num_edicts = num_edicts;
	
	// mark all clients as unconnected
	for (int i = 0 ; i < game.maxclients; i++)
	{
		entity ent = itoe(i + 1);
		ent.client.pers.connected = false;
	}

	// do any load time things at this point
	for (int i = 0 ; i < globals.num_edicts; i++)
	{
		entity ent = itoe(i);

		if (!ent.inuse)
			continue;

		// fire any cross-level triggers
		if (ent.classname == "target_crosslevel_target")
			ent.nextthink = level.framenum + (int)(ent.delay * BASE_FRAMERATE);
	}
#endif
}

extern "C" game_export *GetGameAPI(game_import_impl *impl)
{
	gi.set_impl(impl);

	ge.edicts = gi.TagMalloc<entity>(ge.max_edicts, TAG_GAME);

	// s.number and client must always be assigned
	for (auto &e : entity_range(0, max_entities - 1))
		e.s.number = &e - ge.edicts;

	world = ge.edicts[0];

	return &ge;
}
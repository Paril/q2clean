#include "config.h"
#include "entity.h"
#include "spawn.h"
#include "game.h"
#include "player.h"

#include "lib/gi.h"
#include "game.h"
#include "util.h"
#include "lib/string/format.h"
#ifdef SINGLE_PLAYER
#include "trail.h"
#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif
#endif
#include "items/itemlist.h"
#include "game/items/entity.h"
#include "game/statusbar.h"
#include "game/func.h"
#include "game/trigger.h"

static const registered_entity *registered_entities_head;

/*static*/ void spawnable_entities::register_entity(registered_entity &ent)
{
	ent.next = registered_entities_head;
	registered_entities_head = &ent;
}

stringlit FindClassnameFromEntityType(const entity_type &type)
{
	for (auto e = registered_entities_head; e; e = e->next)
		if (e->type.is_exact(type))
			return e->classname;

	return nullptr;
}

using spawn_deserializer = bool(*)(const string &input, void *obj);

struct spawn_field
{
	stringlit			key;
	spawn_deserializer	func;
	bool				is_temp;
};

static stringref strip_newlines(const string &input)
{
	if (!input)
		return input;

	size_t len = 0;

	for (auto it = input.ptr(); *it; it++)
	{
		if (*it == '\\')
			it++;

		len++;
	}

	if (len == input.length())
		return input;

	mutable_string str;
	str.reserve(len + 1);

	for (auto it = input.ptr(); *it; it++)
	{
		if (*it == '\\')
		{
			it++;

			if (*it == 'n')
				str += '\n';
			else
				str += '\\';
		}
		else
			str += *it;
	}

	return std::move(str);
}

template<typename T>
concept is_string_like = std::is_same_v<T, string> || std::is_same_v<T, stringref>;

template<is_string_like T>
static bool deserialize(const string &input, T &output)
{
	output = strip_newlines(input);
	return true;
}

template<typename T> requires std::is_integral_v<T>
static bool deserialize(const string &input, T &output)
{
	if (!input) {
		output = (T) 0;
		return true;
	}

	char *endptr;

	if constexpr(std::is_unsigned_v<T>)
	{
		if constexpr(sizeof(T) > 4)
			output = (T)strtoull(input.ptr(), (char **)&endptr, 10);
		else
			output = (T)strtoul(input.ptr(), (char **)&endptr, 10);
	}
	else
	{
		if constexpr(sizeof(T) > 4)
			output = (T)strtoll(input.ptr(), (char **)&endptr, 10);
		else
			output = (T)strtol(input.ptr(), (char **)&endptr, 10);
	}

	return endptr;
}

template<typename T> requires std::is_enum_v<T>
static bool deserialize(const string &input, T &output)
{
	return deserialize(input, (std::underlying_type_t<T> &) output);
}

template<typename T>
concept is_floating_like = std::is_floating_point_v<T>;

template<is_floating_like T>
static bool deserialize(const string &input, T &output)
{
	if (!input) {
		output = (T) 0;
		return true;
	}

	char *endptr;

	if constexpr(sizeof(T) > 4)
		output = (T)strtod(input.ptr(), (char **)&endptr);
	else
		output = (T)strtof(input.ptr(), (char **)&endptr);

	return endptr;
}

template<typename T> requires std::is_same_v<T, gtimef>
static bool deserialize(const string &input, T &output)
{
	float v;

	if (!deserialize<float>(input, v))
		return false;

	output = gtimef(v);
	return true;
}

static bool deserialize(const string &input, vector &output)
{
	char *endptr = (char *)input.ptr();

	for (size_t i = 0; i < 3; i++)
	{
		output[i] = strtof(endptr, (char **)&endptr);

		if (!endptr)
			return false;
	}

	return true;
}

#define SPAWN_EFIELD_NAMED(name, field) \
	{ name, [](const string &input, void *obj) { return deserialize(input, ((entity *) obj)->field); }, false }

#define SPAWN_EFIELD(name) \
	SPAWN_EFIELD_NAMED(#name, name)

#define SPAWN_TFIELD_NAMED(name, field) \
	{ name, [](const string &input, void *obj) { return deserialize(input, ((spawn_temp *) obj)->field); }, true }

#define SPAWN_TFIELD(name) \
	SPAWN_TFIELD_NAMED(#name, name)

constexpr spawn_field spawn_fields[] =
{
	// entity fields
	SPAWN_EFIELD(model),
	SPAWN_EFIELD(spawnflags),
	SPAWN_EFIELD(speed),
	SPAWN_EFIELD(accel),
	SPAWN_EFIELD(decel),
	SPAWN_EFIELD(target),
	SPAWN_EFIELD(targetname),
	SPAWN_EFIELD(pathtarget),
	SPAWN_EFIELD(deathtarget),
	SPAWN_EFIELD(killtarget),
	SPAWN_EFIELD(combattarget),
	SPAWN_EFIELD(message),
	SPAWN_EFIELD(team),
	SPAWN_EFIELD(wait),
	SPAWN_EFIELD(delay),
	SPAWN_EFIELD_NAMED("random", rand),
	SPAWN_EFIELD(move_origin),
	SPAWN_EFIELD(move_angles),
	SPAWN_EFIELD(style),
	SPAWN_EFIELD(count),
	SPAWN_EFIELD(health),
	SPAWN_EFIELD(sounds),
	SPAWN_EFIELD(dmg),
	SPAWN_EFIELD(mass),
	SPAWN_EFIELD(volume),
	SPAWN_EFIELD(attenuation),
	SPAWN_EFIELD(map),
	SPAWN_EFIELD(origin),
	SPAWN_EFIELD(angles),
	SPAWN_EFIELD_NAMED("angle", angles.y),
	{ "light", nullptr, false },

	// spawntemp fields
	SPAWN_TFIELD(classname),

	SPAWN_TFIELD(sky),
	SPAWN_TFIELD(skyrotate),
	SPAWN_TFIELD(skyaxis),
	SPAWN_TFIELD(nextmap),

	SPAWN_TFIELD(lip),
	SPAWN_TFIELD(distance),
	SPAWN_TFIELD(height),
	SPAWN_TFIELD(noise),
	SPAWN_TFIELD(pausetime),
	SPAWN_TFIELD(item),
	SPAWN_TFIELD(gravity),
	
	SPAWN_TFIELD(minyaw),
	SPAWN_TFIELD(maxyaw),
	SPAWN_TFIELD(minpitch),
	SPAWN_TFIELD(maxpitch)
};

static bool ED_ParseField(const string &key, const string &value, entity &ent)
{
	for (auto &field : spawn_fields)
	{
		if (!striequals(field.key, key))
			continue;

		if (field.func)
			field.func(value, field.is_temp ? (void *) &st : (void *) &ent);

		return true;
	}
	
	return false;
}

static void ClearSpawnTemp()
{
	st = {};
}

static void ED_ParseEdict(stringlit entities, size_t &entities_offset, entity &ent)
{
	bool init = false;
	
	// go through all the dictionary pairs
	while (true)
	{
		// parse key
		string key = strtok(entities, entities_offset);
		
		if (key == "}")
			break;
		else if (strempty(key))
			gi.errorfmt("{}: EOF without closing brace");

		init = true;
		
		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (key[0] == '_')
		{
			strtok(entities, entities_offset);
			continue;
		}

		string value = strtok(entities, entities_offset);
		
		if (!ED_ParseField(key, value, ent))
			gi.dprintfmt("{}: {} is not a field\n", __func__, key);
	}
	
	if (!init)
		G_FreeEdict(ent);
};

// these are only used by the spawn function
#ifdef SINGLE_PLAYER
constexpr spawn_flag SPAWNFLAG_NOT_EASY		= (spawn_flag)0x00000100;
constexpr spawn_flag SPAWNFLAG_NOT_MEDIUM	= (spawn_flag)0x00000200;
constexpr spawn_flag SPAWNFLAG_NOT_HARD		= (spawn_flag)0x00000400;
#endif
constexpr spawn_flag SPAWNFLAG_NOT_DEATHMATCH	= (spawn_flag)0x00000800;
#if defined(SINGLE_PLAYER) && defined(GROUND_ZERO)
constexpr spawn_flag SPAWNFLAG_NOT_COOP		= (spawn_flag)0x00001000;
#endif
constexpr spawn_flag SPAWNFLAG_NOT_MASK		= (spawn_flag)0x00001F00; // mask of all of the NOT_ flags

void PreSpawnEntities()
{
#ifdef SINGLE_PLAYER
	SaveClientData();
#endif
}

bool ED_CallSpawn(entity &ent)
{
	// if we have a type, call it immediately
	if (ent.type)
	{
		if (ent.type->spawn->func)
		{
			ent.type->spawn->func(ent);
			return true;
		}

		G_FreeEdict(ent);
		return true;
	}

	// no type, so determine it from classname
	if (!st.classname)
	{
		gi.dprintfmt("{}: NULL classname\n", __func__);
		G_FreeEdict(ent);
		return false;
	}
	
#ifdef GROUND_ZERO
	// FIXME: dis succ
	if (st.classname == "weapon_nailgun")
		st.classname = GetItemByIndex(ITEM_ETF_RIFLE).classname;
	if (st.classname == "ammo_nails")
		st.classname = GetItemByIndex(ITEM_FLECHETTES).classname;
	if (st.classname == "weapon_heatbeam")
		st.classname = GetItemByIndex(ITEM_PLASMA_BEAM).classname;
#endif
	
	// check item spawn functions
	itemref item = FindItemByClassname(st.classname);
	
	if (item.has_value())
	{
		SpawnItem(ent, item);
		return true;
	}

	for (auto spawn = registered_entities_head; spawn; spawn = spawn->next)
	{
		if (striequals(spawn->classname, st.classname))
		{
			ent.type = spawn->type;
			return ED_CallSpawn(ent);
		}
	}

	gi.dprintfmt("{}: {} doesn't have a spawn function\n", __func__, st.classname);
	G_FreeEdict(ent);
	return false;
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
// Paril: it took me a while to sus out what exactly this function was for in
// Ground Zero, but basically it was to fix their mappers' bad teams. Maybe they
// were broken later on or grouped wrong and it caused issues? Anyways,
// basically it re-ordered teams with trains so that a train was always
// at the front and synced up their speeds.
static inline void G_FixTeams()
{
	size_t c = 0;

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);

		if (!e.teammaster.has_value())
			continue;
		
		entity &master = e.teammaster;

		// already a train, so don't worry about us
		if (master.type == ET_FUNC_TRAIN)
			continue;

		entityref first_train;

		for (entity &e2 : G_IterateChain<&entity::teamchain>(master))
		{
			if (e2.type == ET_FUNC_TRAIN)
			{
				first_train = e2;
				break;
			}
		}

		// no train in this team
		if (!first_train.has_value())
			continue;

		c++;
		
		entityref prev;

		// got one! we have to reposition this train to be the master now.
		// swap all of the masters out, and store the guy before the train
		for (entity &e2 : G_IterateChain<&entity::teamchain>(master))
		{
			e2.teammaster = first_train;

			// copy over movetype and speed
			e2.movetype = MOVETYPE_PUSH;
			e2.speed = first_train->speed;
			
			// reached the guy before first_train
			if (e2.teamchain == first_train)
				prev = e2;
		}
		
		// shim us in
		if (!prev.has_value())
			gi.error("wat");

		// master = old teammaster
		// prev = guy before first_train
		// first_train = first.. train

		prev->teamchain = master;
		entity &old_master_next = master.teamchain;
		master.teamchain = first_train->teamchain;
		first_train->teamchain = old_master_next;
		
		// remove our slave flag
		first_train->flags &= ~FL_TEAMSLAVE;
		
		// give the old master the flag
		master.flags |= FL_TEAMSLAVE;
	}

	gi.dprintfmt("{} teams repaired\n", c);
}

static inline void G_FindTeams()
{
	size_t c = 0, c2 = 0;

	for (uint32_t i = 1; i < num_entities; i++)
	{
		entity &e = itoe(i);
		 
		if (!e.inuse)
			continue;
		if (!e.team)
			continue;
		if (e.flags & FL_TEAMSLAVE)
			continue;

		entityref chain = e;
		e.teammaster = e;
		e.teamchain = 0;
		c++;
		c2++;

		for (uint32_t j = i + 1; j < num_entities ; j++)
		{
			entity &e2 = itoe(j);

			if (!e2.inuse)
				continue;
			if (!e2.team)
				continue;
			if (e2.flags & FL_TEAMSLAVE)
				continue;
			if (e.team == e2.team)
			{
				c2++;
				chain->teamchain = e2;
				e2.teammaster = e;
				e2.teamchain = 0;
				chain = e2;
				e2.flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintfmt("{} teams with {} entities\n", c, c2);
	
	G_FixTeams();
}

void SpawnEntities(stringlit mapname, stringlit entities, stringlit spawnpoint)
{
	size_t entities_offset = 0;

#ifdef SINGLE_PLAYER
	int32_t skill_level = clamp(0, (int32_t)skill, 3);

	if (skill != skill_level)
		gi.cvar_forcesetfmt("skill", "{}", skill_level);
#endif

	level = {};
	level.mapname = mapname;
	game.spawnpoint = spawnpoint;
	
	entityref ent = world;
	size_t inhibit = 0;
	
	// parse ents
	while (1)
	{
		string token = strtok(entities, entities_offset);
		
		if (entities_offset == (size_t) -1)
			break;

		if (token != "{")
			gi.errorfmt("{}: found {} when expecting {", __func__, token);
		
		if (ent->inuse)
			ent = G_Spawn();
		else
			G_InitEdict(ent);
		
		ED_ParseEdict(entities, entities_offset, ent);

#ifdef SINGLE_PLAYER
		// yet another map hack
		if (striequals(level.mapname, "command") && ent->type == ET_TRIGGER_ONCE && striequals(ent->model, "*27"))
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;
#endif
		// remove things (except the world) from different skill levels or deathmatch
		if (!ent.is_world())
		{
#ifdef SINGLE_PLAYER
			if (deathmatch)
			{
#endif
				if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
#ifdef SINGLE_PLAYER
			}
			else
			{
#ifdef GROUND_ZERO
				if (coop && (ent->spawnflags & SPAWNFLAG_NOT_COOP))
				{
					G_FreeEdict (ent);	
					inhibit++;
					continue;
				}
				
				// TODO: there was a block here about (EASY | MED | HARD) meaning that it's coop-only.
				// is this true in only rogue maps?
#endif

				if (((skill_level == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill_level == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill_level == 2) || (skill_level == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD)))
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}
#endif
			ent->spawnflags &= ~SPAWNFLAG_NOT_MASK;
		}

		if (!ED_CallSpawn(ent))
			inhibit++;
#ifdef GROUND_ZERO
		else
			ent->renderfx |= RF_IR_VISIBLE;
#endif
	
		ClearSpawnTemp();
	}
	
	ClearSpawnTemp();

	gi.dprintfmt("{} entities inhibited\n", inhibit);

	G_FindTeams();
#ifdef SINGLE_PLAYER

	PlayerTrail_Init();

#ifdef ROGUE_AI
	if (!deathmatch)
		InitHintPaths();
#endif
#endif

#ifdef CTF
	CTFSpawn();
#endif
}

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
"sounds"    music cd track number
"gravity"   800 is default gravity
"message"   text to print at user logon
*/
static void SP_worldspawn(entity &ent)
{
	ent.movetype = MOVETYPE_PUSH;
	ent.solid = SOLID_BSP;
	ent.modelindex = MODEL_WORLD;      // world model is always index 1

	//---------------

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue();

	// set configstrings for items
	for (auto &it : item_list())
		gi.configstring((config_string) (CS_ITEMS + (config_string) it.id), it.pickup_name);

	if (st.nextmap)
		level.nextmap = st.nextmap;

	// make some data visible to the server

	if (ent.message)
	{
		gi.configstring(CS_NAME, ent.message);
		level.level_name = ent.message;
	}
	else
		level.level_name = level.mapname;

	if (st.sky)
		gi.configstring(CS_SKY, st.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstringfmt(CS_SKYROTATE, "{}", st.skyrotate);

	gi.configstringfmt(CS_SKYAXIS, "{}", st.skyaxis);

	gi.configstringfmt(CS_CDTRACK, "{}", ent.sounds);

	gi.configstringfmt(CS_MAXCLIENTS, "{}", game.maxclients);

	// status bar program
	gi.configstring(CS_STATUSBAR, G_GetStatusBar());

#ifdef CTF
	if (ctf)
		CTFPrecache();
#endif

	// help icon for statusbar
	gi.imageindex("i_help");
	level.pic_health = gi.imageindex("i_health");
	gi.imageindex("help");
	gi.imageindex("field_3");

	if (!st.gravity)
		gi.cvar_set("sv_gravity", "800");
	else
		gi.cvar_set("sv_gravity", st.gravity);

	snd_fry = gi.soundindex("player/fry.wav");  // standing in lava / slime

	PrecacheItem(GetItemByIndex(ITEM_BLASTER));

	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");

	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");

	gi.soundindex("misc/udeath.wav");

	// gibs
	gi.soundindex("items/respawn1.wav");

	// sexed sounds
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");
	gi.soundindex("*gurp1.wav");        // drowning damage
	gi.soundindex("*gurp2.wav");
	gi.soundindex("*jump1.wav");        // player jump
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");

	// sexed models
	InitVWeps();

	gi.soundindex("player/gasp1.wav");      // gasping for air
	gi.soundindex("player/gasp2.wav");      // head breaking surface, not gasping

	gi.soundindex("player/watr_in.wav");    // feet hitting water
	gi.soundindex("player/watr_out.wav");   // feet leaving water

	gi.soundindex("player/watr_un.wav");    // head going underwater

	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");

	gi.soundindex("items/pkup.wav");        // bonus item pickup
	gi.soundindex("world/land.wav");        // landing thud
	gi.soundindex("misc/h2ohit1.wav");      // landing splash

	gi.soundindex("items/damage.wav");
#ifdef GROUND_ZERO
	gi.soundindex ("misc/ddamage1.wav");
#endif
	gi.soundindex("items/protect.wav");
	gi.soundindex("items/protect4.wav");
	gi.soundindex("weapons/noammo.wav");

#ifdef SINGLE_PLAYER
	gi.soundindex("infantry/inflies1.wav");
#endif

	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/arm/tris.md2");
	gi.modelindex("models/objects/gibs/bone/tris.md2");
	gi.modelindex("models/objects/gibs/chest/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");
	gi.modelindex("models/objects/gibs/head2/tris.md2");

//
// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
//

	// 0 normal
	gi.configstring((config_string)(CS_LIGHTS + 0), "m");

	// 1 FLICKER (first variety)
	gi.configstring((config_string)(CS_LIGHTS + 1), "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	gi.configstring((config_string)(CS_LIGHTS + 2), "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	gi.configstring((config_string)(CS_LIGHTS + 3), "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	gi.configstring((config_string)(CS_LIGHTS + 4), "mamamamamama");

	// 5 GENTLE PULSE 1
	gi.configstring((config_string)(CS_LIGHTS + 5), "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	gi.configstring((config_string)(CS_LIGHTS + 6), "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	gi.configstring((config_string)(CS_LIGHTS + 7), "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	gi.configstring((config_string)(CS_LIGHTS + 8), "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	gi.configstring((config_string)(CS_LIGHTS + 9), "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	gi.configstring((config_string)(CS_LIGHTS + 10), "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring((config_string)(CS_LIGHTS + 11), "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configstring((config_string)(CS_LIGHTS + 63), "a");
}

static REGISTER_ENTITY(WORLDSPAWN, worldspawn);
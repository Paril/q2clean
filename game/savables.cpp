#include "savables.h"

#ifdef SAVING
#include <fstream>
#include "lib/string/format.h"
#include "game/items/itemlist.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/player.h"
#include "lib/math/bbox.h"

#ifdef SINGLE_PLAYER
#include "game/target.h"
#endif

registered_save_data<void *> *registered_data_head;
registered_save_function<void *> *registered_functions_head;

#ifdef JSON_SAVE_FORMAT
#define CJSON_HIDE_SYMBOLS
#include "debug/cJSON.h"
#endif

#ifdef JSON_SAVE_FORMAT
using serializer = struct json_serializer;

using write_func = cJSON *(*)(serializer &, const void *);
using read_func = void(*)(cJSON *, serializer &, void *);
#else
using serializer = struct binary_serializer;

using write_func = void(*)(serializer &, const void *);
using read_func = void(*)(serializer &, void *);
#endif

struct save_member
{
	const stringlit		name;
	write_func			write;
	read_func			read;
};

struct save_struct
{
	const stringlit		name;
	const save_member	*members;
	const size_t		num_members;
};

#ifdef JSON_SAVE_FORMAT
struct json_serializer
{
	cJSON *json;
	bool load;
	stringref filename;
	
	json_serializer(stringref filename, bool load) :
		json(nullptr),
		load(load),
		filename(filename)
	{	
		if (load)
		{
			std::fstream f(filename.ptr(), std::fstream::in);

			if (!f.is_open())
				gi.error("No file");

			f.seekg(0, std::ios::end);   
			string str(f.tellg());
			f.seekg(0, std::ios::beg);

			char *o = (char *) str.ptr();
			
			for (auto it = std::istreambuf_iterator<char>(f); it != std::istreambuf_iterator<char>(); it++)
				*o++ = *it;

			*o++ = 0;

			json = cJSON_Parse(str.ptr());
		}
		else
		{
			json = cJSON_CreateObject();
		}
	}

	~json_serializer()
	{
		if (!load)
		{
			char *js = cJSON_Print(json);
			std::fstream f(filename.ptr(), std::fstream::out | std::fstream::trunc);
			f.write(js, strlen(js));
			free(js);
		}

		cJSON_Delete(json);
	}

	cJSON *write_struct(const save_struct &struc, const void *ptr, const bool &defaultable = false)
	{
		cJSON *object = cJSON_CreateObject();

		for (auto member = struc.members; member != struc.members + struc.num_members; member++)
		{
			cJSON* obj = member->write(*this, ptr);

			if (obj)
				cJSON_AddItemToObject(object, member->name, obj);
		}

		if (defaultable && !object->child)
		{
			cJSON_Delete(object);
			return nullptr;
		}

		return object;
	}

	void read_struct(cJSON *obj, const save_struct &struc, void *ptr)
	{
		for (auto child = obj->child; child; child = child->next)
		{
			for (auto member = struc.members; member != struc.members + struc.num_members; member++)
			{
				if (strcmp(child->string, member->name))
					continue;

				member->read(child, *this, ptr);
				break;
			}
		}
	}
};

// The following types can be serialized to JSON as numbers
template<typename T>
static constexpr bool is_number = !std::is_pointer_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>);
	
template<typename T> requires is_number<T>
inline cJSON *json_serializer_write(serializer&, const T &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateNumber((double) v);
}

template<typename T> requires std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat>
inline cJSON *json_serializer_write(serializer&, const T &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateNumber((double) (int32_t) v);
}

inline cJSON *json_serializer_write(serializer&, const bool &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateTrue();
}

inline cJSON *json_serializer_write(serializer&, const stringref &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v.ptr());
}

inline cJSON *json_serializer_write(serializer&, const string &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v.ptr());
}

inline cJSON *json_serializer_write(serializer&, const entityref &v, const bool& defaultable = false)
{
	if (defaultable && !v.has_value())
		return nullptr;

	return cJSON_CreateNumber((double)v->s.number);
}

inline cJSON *json_serializer_write(serializer&, const itemref &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v->classname);
}

inline cJSON *json_serializer_write(serializer&, const entity_type_ref &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v->id);
}

inline cJSON *json_serializer_write(serializer&, const vector &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateFloatArray(&v.x, 3);
}

inline cJSON *json_serializer_write(serializer&, const bbox &v, const bool& defaultable = false)
{
	if (defaultable && !v.mins && !v.maxs)
		return nullptr;

	return cJSON_CreateFloatArray(&v.mins.x, 6);
}

template<typename TFunc>
inline cJSON *json_serializer_write(serializer&, const savable_function<TFunc> &str, const bool& defaultable = false)
{
	if (str)
		return cJSON_CreateString(str.func->name);

	return defaultable ? nullptr : cJSON_CreateStringReference("");
}

template<typename T>
inline cJSON *json_serializer_write(serializer&, const savable_data<T> &str, const bool& defaultable = false)
{
	if (str)
		return cJSON_CreateString(str.data->name);

	return defaultable ? nullptr : cJSON_CreateStringReference("");
}
	
template<typename T, size_t N>
inline cJSON *json_serializer_write(serializer &stream, const array<T, N> &arr, const bool &defaultable = false)
{
	static array<T, N> default_val;

	if (defaultable && arr == default_val)
		return nullptr;

	cJSON *array = cJSON_CreateArray();

	for (auto &v : arr)
		cJSON_AddItemToArray(array, json_serializer_write(stream, v));

	return array;
}

template<typename T> requires is_number<T>
inline void json_serializer_read(cJSON *json, serializer &, T &str)
{
	str = (T) cJSON_GetNumberValue(json);
}

template<typename T> requires std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat>
inline void json_serializer_read(cJSON *json, serializer &, T &str)
{
	str = (T) (int32_t) cJSON_GetNumberValue(json);
}

inline void json_serializer_read(cJSON *json, serializer &, bool &str)
{
	str = cJSON_IsTrue(json) ? true : false;
}

inline void json_serializer_read(cJSON *json, serializer &, stringref &str)
{
	str = cJSON_GetStringValue(json);
}

inline void json_serializer_read(cJSON *json, serializer &, string &str)
{
	str = cJSON_GetStringValue(json);
}

inline void json_serializer_read(cJSON *json, serializer &, entityref &str)
{
	if (cJSON_IsNull(json))
		str = entityref();
	else
		str = itoe((size_t) cJSON_GetNumberValue(json));
}

inline void json_serializer_read(cJSON *json, serializer &, itemref &str)
{
	str = FindItemByClassname(cJSON_GetStringValue(json));
}

inline void json_serializer_read(cJSON *json, serializer &, entity_type_ref &str)
{
	const char *s = cJSON_GetStringValue(json);

	for (const entity_type *x = &ET_UNKNOWN; x; x = x->next)
	{
		if (strcmp(x->id, s) == 0)
		{
			str = *x;
			return;
		}
	}

	gi.dprintf("Warning: unknown entity type %s\n", s);
	str = ET_UNKNOWN;
}

inline void json_serializer_read(cJSON *json, serializer &, vector &str)
{
	for (int32_t i = 0; i < 3; i++)
		str[i] = (float) cJSON_GetNumberValue(cJSON_GetArrayItem(json, i));
}

inline void json_serializer_read(cJSON *json, serializer &, bbox &str)
{
	for (int32_t i = 0; i < 6; i++)
		(i < 3 ? str.mins : str.maxs)[i % 3] = (float) cJSON_GetNumberValue(cJSON_GetArrayItem(json, i));
}

template<typename T, size_t N>
inline void json_serializer_read(cJSON *json, serializer &stream, array<T, N> &arr)
{
	int i = 0;

	for (auto &v : arr)
		json_serializer_read(cJSON_GetArrayItem(json, i++), stream, v);
}

template<typename TFunc>
inline void json_serializer_read(cJSON *json, serializer &stream, savable_function<TFunc> &str)
{
	string s;

	json_serializer_read(json, stream, s);

	if (!s)
	{
		str = nullptr;
		return;
	}

	for (auto p = registered_functions_head; p; p = p->next)
	{
		if (s == p->name)
		{
			savable_function<void *> t(*p);
			str = *(savable_function<TFunc> *)&t;
			return;
		}
	}

	gi.error("No function matching %s\n", s.ptr());
}
	
template<typename T>
inline void json_serializer_read(cJSON *json, serializer &stream, savable_data<T> &str)
{
	string s;
	
	json_serializer_read(json, stream, s);

	if (!s)
	{
		str = nullptr;
		return;
	}

	for (auto p = registered_data_head; p; p = p->next)
	{
		if (s == p->name)
		{
			savable_data<void *> t(*p);
			str = *(savable_data<T> *)&t;
			return;
		}
	}

	gi.error("No data matching %s\n", s.ptr());
}
#else
struct binary_serializer
{
	// The following types can be written/read directly without any extra work.
	template<typename T>
	static constexpr bool is_trivial = !std::is_pointer_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T> ||
		std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat> ||
		std::is_same_v<T, vector> || std::is_same_v<T, bbox> || std::is_same_v<T, pmove_state> || std::is_same_v<T, player_state> || std::is_same_v<T, entity_state>);

	std::fstream stream;

	binary_serializer(stringlit filename, bool load) :
		stream(filename, std::fstream::binary | (load ? std::fstream::in : std::fstream::out | std::fstream::trunc))
	{
	}

	// write routines
	template<typename T> requires is_trivial<T>
	inline void operator<<(const T &str)
	{
		stream.write((char *) &str, sizeof(T));
	}

	inline void operator<<(const stringref &str)
	{
		uint32_t len;

		if (!str)
			len = 0;
		else
			len = str.length();
	
		stream.write((char *) &len, sizeof(len));

		if (len)
			stream.write(str.ptr(), len);
	}

	inline void operator<<(const string &str)
	{
		this->operator<<(stringref(str));
	}

	inline void operator<<(const itemref &str)
	{
		stream.write((char *) &str->id, sizeof(str->id));
	}

	inline void operator<<(const entityref &str)
	{
		int32_t entity_id = str.has_value() ? str->s.number : -1;

		stream.write((char *) &entity_id, sizeof(entity_id));
	}

	template<typename T, size_t N>
	inline void operator<<(const array<T, N> &arr)
	{
		for (auto &v : arr)
			this->operator<<(v);
	}

	template<typename TFunc>
	inline void operator<<(const savable_function<TFunc> &str)
	{
		this->operator<<(stringref(str ? str.func->name : ""));
	}

	template<typename T>
	inline void operator<<(const savable_data<T> &str)
	{
		this->operator<<(stringref(str ? str.data->name : ""));
	}

	// read operators
	template<typename T> requires is_trivial<T>
	inline void operator>>(T &str)
	{
		stream.read((char *) &str, sizeof(T));
	}

	inline void operator>>(stringref &str)
	{
		string s;

		this->operator>>(s);

		str = std::move(s);
	}

	inline void operator>>(string &str)
	{
		uint32_t len;

		stream.read((char *) &len, sizeof(len));

		if (len)
		{
			str = string(len);
			stream.read((char *) str.ptr(), len);
			((char *) str.ptr())[len] = 0;
		}
		else
			str = string();
	}

	inline void operator>>(itemref &str)
	{
		gitem_id id;

		stream.read((char *) &id, sizeof(id));

		str = itemref(id);
	}

	inline void operator>>(entityref &str)
	{
		int32_t entity_id;

		stream.read((char *) &entity_id, sizeof(entity_id));

		if (entity_id == -1)
			str = entityref();
		else
			str = itoe((size_t) entity_id);
	}
	
	template<typename T, size_t N>
	inline void operator>>(array<T, N> &arr)
	{
		for (auto &v : arr)
			this->operator>>(v);
	}
	
	template<typename TFunc>
	inline void operator>>(savable_function<TFunc> &str)
	{
		string s;

		this->operator>>(s);

		if (!s)
		{
			str = nullptr;
			return;
		}

		for (auto p = registered_functions_head; p; p = p->next)
		{
			if (s == p->name)
			{
				savable_function<void *> t(*p);
				str = *(savable_function<TFunc> *)&t;
				return;
			}
		}

		gi.error("No function matching %s\n", s.ptr());
	}
	
	template<typename T>
	inline void operator>>(savable_data<T> &str)
	{
		string s;

		this->operator>>(s);

		if (!s)
		{
			str = nullptr;
			return;
		}

		for (auto p = registered_data_head; p; p = p->next)
		{
			if (s == p->name)
			{
				savable_data<void *> t(*p);
				str = *(savable_data<T> *)&t;
				return;
			}
		}

		gi.error("No data matching %s\n", s.ptr());
	}

	// structs
	void write_struct(const save_struct &struc, const void *ptr)
	{
		for (auto member = struc.members; member != struc.members + struc.num_members; member++)
			member->write(*this, ptr);
	}

	void read_struct(const save_struct &struc, void *ptr)
	{
		for (auto member = struc.members; member != struc.members + struc.num_members; member++)
			member->read(*this, ptr);
	}
};
#endif

#ifdef JSON_SAVE_FORMAT
#define SAVE_MEMBER(type, member) \
	{ #member, \
		[](serializer &stream, const void *struc) { return json_serializer_write(stream, ((type *) struc)->member, true); }, \
		[](cJSON *json, serializer &stream, void *struc) { json_serializer_read(json, stream, ((type *) struc)->member); } }

// A few things in our code rely on getters/setters to hide
// protocol-based behavior.
#define SAVE_MEMBER_PROPERTY(type, member) \
	{ #member, \
		[](serializer &stream, const void *struc) { return json_serializer_write(stream, ((type *) struc)->get_##member(), true); }, \
		[](cJSON *json, serializer &stream, void *struc) { std::invoke_result_t<decltype(&type::get_##member), type> temp; json_serializer_read(json, stream, temp); ((type *) struc)->set_##member(temp); } }
#else
#define SAVE_MEMBER(type, member) \
	{ #member, \
		[](serializer &stream, const void *struc) { stream << ((type *) struc)->member; }, \
		[](serializer &stream, void *struc) { stream >> ((type *) struc)->member; } }

// A few things in our code rely on getters/setters to hide
// protocol-based behavior.
#define SAVE_MEMBER_PROPERTY(type, member) \
	{ #member, \
		[](serializer &stream, const void *struc) { stream << ((type *) struc)->get_##member(); }, \
		[](serializer &stream, void *struc) { std::invoke_result_t<decltype(&type::get_##member), type> temp; stream >> temp; ((type *) struc)->set_##member(temp); } }
#endif

#define DEFINE_SAVE_STRUCTURE(struct_name) \
	static save_struct struct_name##_save = { \
		#struct_name, \
		struct_name##_members, \
		lengthof(struct_name##_members) \
	}

#ifdef JSON_SAVE_FORMAT
#define CREATE_STRUCTURE_SERIALIZE_FUNCS(type) \
	DEFINE_SAVE_STRUCTURE(type); \
	inline cJSON *json_serializer_write(serializer &stream, const type &str, const bool &) { return stream.write_struct(type##_save, &str, true); } \
	inline void json_serializer_read(cJSON *json, serializer &stream, type &str) { stream.read_struct(json, type##_save, &str); }
#else
#define CREATE_STRUCTURE_SERIALIZE_FUNCS(type) \
	DEFINE_SAVE_STRUCTURE(type); \
	inline void operator<<(serializer &stream, const type &str) { stream.write_struct(type##_save, &str); } \
	inline void operator>>(serializer &stream, type &str) { stream.read_struct(type##_save, &str); }
#endif

static save_member game_locals_members[] = {
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(game_locals, helpmessage1),
	SAVE_MEMBER(game_locals, helpmessage2),
	SAVE_MEMBER(game_locals, helpchanged),
	SAVE_MEMBER(game_locals, serverflags),
	SAVE_MEMBER(game_locals, autosaved),
#endif
	SAVE_MEMBER(game_locals, spawnpoint),
	SAVE_MEMBER(game_locals, maxclients)
};

DEFINE_SAVE_STRUCTURE(game_locals);

static save_member level_locals_members[] = {
	SAVE_MEMBER(level_locals, framenum),
	SAVE_MEMBER(level_locals, time),
	
	SAVE_MEMBER(level_locals, level_name),
	SAVE_MEMBER(level_locals, mapname),
	SAVE_MEMBER(level_locals, nextmap),
	
	SAVE_MEMBER(level_locals, intermission_framenum),
	SAVE_MEMBER(level_locals, changemap),
	SAVE_MEMBER(level_locals, exitintermission),
	SAVE_MEMBER(level_locals, intermission_origin),
	SAVE_MEMBER(level_locals, intermission_angle),

	SAVE_MEMBER(level_locals, pic_health),
	
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(level_locals, sight_client),
	
	SAVE_MEMBER(level_locals, sight_entity),
	SAVE_MEMBER(level_locals, sight_entity_framenum),
	SAVE_MEMBER(level_locals, sound_entity),
	SAVE_MEMBER(level_locals, sound_entity_framenum),
	SAVE_MEMBER(level_locals, sound2_entity),
	SAVE_MEMBER(level_locals, sound2_entity_framenum),

	SAVE_MEMBER(level_locals, total_secrets),
	SAVE_MEMBER(level_locals, found_secrets),
	
	SAVE_MEMBER(level_locals, total_goals),
	SAVE_MEMBER(level_locals, found_goals),
	
	SAVE_MEMBER(level_locals, total_monsters),
	SAVE_MEMBER(level_locals, killed_monsters),

	SAVE_MEMBER(level_locals, power_cubes),
#endif
	
	SAVE_MEMBER(level_locals, current_entity),
	SAVE_MEMBER(level_locals, body_que)
};

DEFINE_SAVE_STRUCTURE(level_locals);

static save_member client_persistant_members[] = {
	SAVE_MEMBER(client_persistant, userinfo),
	SAVE_MEMBER(client_persistant, netname),
	SAVE_MEMBER(client_persistant, hand),

	SAVE_MEMBER(client_persistant, connected),
	
	SAVE_MEMBER(client_persistant, selected_item),
	SAVE_MEMBER(client_persistant, inventory),
	
	SAVE_MEMBER(client_persistant, max_ammo),

	SAVE_MEMBER(client_persistant, weapon),
	SAVE_MEMBER(client_persistant, lastweapon),

#ifdef SINGLE_PLAYER
	SAVE_MEMBER(client_persistant, health),
	SAVE_MEMBER(client_persistant, max_health),
	SAVE_MEMBER(client_persistant, power_cubes),
	SAVE_MEMBER(client_persistant, savedFlags),
	SAVE_MEMBER(client_persistant, helpchanged),
	SAVE_MEMBER(client_persistant, game_helpchanged),
	SAVE_MEMBER(client_persistant, score),
#endif

	SAVE_MEMBER(client_persistant, spectator)
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(client_persistant);

static save_member client_respawn_members[] = {
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(client_respawn, coop_respawn),
#endif
	
	SAVE_MEMBER(client_respawn, enterframe),
	SAVE_MEMBER(client_respawn, score),
	SAVE_MEMBER(client_respawn, cmd_angles),

	SAVE_MEMBER(client_respawn, spectator)
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(client_respawn);

static save_member pmove_state_members[] = {
	SAVE_MEMBER(pmove_state, pm_type),
	
	SAVE_MEMBER_PROPERTY(pmove_state, origin),
	SAVE_MEMBER_PROPERTY(pmove_state, velocity),
	SAVE_MEMBER(pmove_state, pm_flags),
	SAVE_MEMBER(pmove_state, pm_time),
	SAVE_MEMBER(pmove_state, gravity),
	SAVE_MEMBER_PROPERTY(pmove_state, delta_angles)
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(pmove_state);

static save_member player_state_members[] = {
	SAVE_MEMBER(player_state, pmove),
	
	SAVE_MEMBER(player_state, viewangles),
	SAVE_MEMBER(player_state, viewoffset),
	SAVE_MEMBER(player_state, kick_angles),
	
	SAVE_MEMBER(player_state, gunangles),
	SAVE_MEMBER(player_state, gunoffset),
	SAVE_MEMBER(player_state, gunindex),
	SAVE_MEMBER(player_state, gunframe),
	
	SAVE_MEMBER(player_state, blend),

	SAVE_MEMBER(player_state, fov),

	SAVE_MEMBER(player_state, rdflags),

	SAVE_MEMBER(player_state, stats),
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(player_state);

static save_member client_members[] = {
	SAVE_MEMBER(client, ps),
	SAVE_MEMBER(client, ping),
	SAVE_MEMBER(client, clientNum),
	
	SAVE_MEMBER(client, pers),
	SAVE_MEMBER(client, resp),
	SAVE_MEMBER(client, old_pmove),
	
	SAVE_MEMBER(client, showscores),
	SAVE_MEMBER(client, showinventory),
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(client, showhelp),
	SAVE_MEMBER(client, showhelpicon),
#endif

	SAVE_MEMBER(client, ammo_index),
	
	SAVE_MEMBER(client, buttons),
	SAVE_MEMBER(client, oldbuttons),
	SAVE_MEMBER(client, latched_buttons),

	SAVE_MEMBER(client, weapon_thunk),

	SAVE_MEMBER(client, newweapon),
	
	SAVE_MEMBER(client, damage_armor),
	SAVE_MEMBER(client, damage_parmor),
	SAVE_MEMBER(client, damage_blood),
	SAVE_MEMBER(client, damage_knockback),
	SAVE_MEMBER(client, damage_from),

	SAVE_MEMBER(client, killer_yaw),

	SAVE_MEMBER(client, weaponstate),
	
	SAVE_MEMBER(client, kick_angles),
	SAVE_MEMBER(client, kick_origin),
	SAVE_MEMBER(client, v_dmg_roll),
	SAVE_MEMBER(client, v_dmg_pitch),
	SAVE_MEMBER(client, v_dmg_time),
	SAVE_MEMBER(client, fall_time),
	SAVE_MEMBER(client, fall_value),
	SAVE_MEMBER(client, bonus_alpha),
	SAVE_MEMBER(client, damage_blend),
	SAVE_MEMBER(client, damage_alpha),
	SAVE_MEMBER(client, v_angle),
	SAVE_MEMBER(client, bobtime),
	SAVE_MEMBER(client, oldviewangles),
	SAVE_MEMBER(client, oldvelocity),
	
	SAVE_MEMBER(client, next_drown_framenum),
	SAVE_MEMBER(client, old_waterlevel),
	SAVE_MEMBER(client, breather_sound),
	
	SAVE_MEMBER(client, machinegun_shots),
	
	SAVE_MEMBER(client, anim_end),
	SAVE_MEMBER(client, anim_priority),
	SAVE_MEMBER(client, anim_duck),
	SAVE_MEMBER(client, anim_run),
	
	SAVE_MEMBER(client, quad_framenum),
	SAVE_MEMBER(client, invincible_framenum),
	SAVE_MEMBER(client, breather_framenum),
	SAVE_MEMBER(client, enviro_framenum),
	
	SAVE_MEMBER(client, grenade_blew_up),
	SAVE_MEMBER(client, grenade_framenum),
	
	SAVE_MEMBER(client, silencer_shots),
	SAVE_MEMBER(client, weapon_sound),

	SAVE_MEMBER(client, pickup_msg_framenum),
	
	SAVE_MEMBER(client, flood_locktill),
	SAVE_MEMBER(client, flood_when),
	SAVE_MEMBER(client, flood_whenhead),

	SAVE_MEMBER(client, respawn_framenum),
	
	SAVE_MEMBER(client, chase_target),
	SAVE_MEMBER(client, update_chase),
	
#ifdef THE_RECKONING
	SAVE_MEMBER(client, quadfire_framenum),
	SAVE_MEMBER(client, trap_framenum),
	SAVE_MEMBER(client, trap_blew_up),
#endif

#ifdef GROUND_ZERO
	SAVE_MEMBER(client, double_framenum),
	SAVE_MEMBER(client, ir_framenum),
	SAVE_MEMBER(client, nuke_framenum),
	SAVE_MEMBER(client, tracker_pain_framenum),
#endif

#ifdef HOOK_CODE
	SAVE_MEMBER(client, grapple),
	SAVE_MEMBER(client, grapplestate),
	SAVE_MEMBER(client, grapplereleaseframenum)
#endif
};

DEFINE_SAVE_STRUCTURE(client);

static save_member entity_state_members[] = {
	SAVE_MEMBER(entity_state, origin),
	SAVE_MEMBER(entity_state, angles),
	SAVE_MEMBER(entity_state, old_origin),

	SAVE_MEMBER(entity_state, modelindex),
	SAVE_MEMBER(entity_state, modelindex2),
	SAVE_MEMBER(entity_state, modelindex3),
	SAVE_MEMBER(entity_state, modelindex4),

	SAVE_MEMBER(entity_state, frame),
	SAVE_MEMBER(entity_state, skinnum),

	SAVE_MEMBER(entity_state, effects),
	SAVE_MEMBER(entity_state, renderfx),

	// solid skipped intentionally

	SAVE_MEMBER(entity_state, sound),

	SAVE_MEMBER(entity_state, event)
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(entity_state);

static save_member moveinfo_members[] = {
	SAVE_MEMBER(moveinfo, start_origin),
	SAVE_MEMBER(moveinfo, start_angles),
	SAVE_MEMBER(moveinfo, end_origin),
	SAVE_MEMBER(moveinfo, end_angles),

	SAVE_MEMBER(moveinfo, sound_start),
	SAVE_MEMBER(moveinfo, sound_middle),
	SAVE_MEMBER(moveinfo, sound_end),

	SAVE_MEMBER(moveinfo, accel),
	SAVE_MEMBER(moveinfo, speed),
	SAVE_MEMBER(moveinfo, decel),
	SAVE_MEMBER(moveinfo, distance),

	SAVE_MEMBER(moveinfo, wait),

	SAVE_MEMBER(moveinfo, state),
	SAVE_MEMBER(moveinfo, dir),
	SAVE_MEMBER(moveinfo, current_speed),
	SAVE_MEMBER(moveinfo, move_speed),
	SAVE_MEMBER(moveinfo, next_speed),
	SAVE_MEMBER(moveinfo, remaining_distance),
	SAVE_MEMBER(moveinfo, decel_distance),

	SAVE_MEMBER(moveinfo, endfunc)
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(moveinfo);

#ifdef SINGLE_PLAYER

static save_member monsterinfo_members[] = {
	SAVE_MEMBER(monsterinfo, currentmove),

	SAVE_MEMBER(monsterinfo, aiflags),
	SAVE_MEMBER(monsterinfo, nextframe),
	SAVE_MEMBER(monsterinfo, scale),

	SAVE_MEMBER(monsterinfo, stand),
	SAVE_MEMBER(monsterinfo, idle),
	SAVE_MEMBER(monsterinfo, search),
	SAVE_MEMBER(monsterinfo, walk),
	SAVE_MEMBER(monsterinfo, run),
	SAVE_MEMBER(monsterinfo, attack),
	SAVE_MEMBER(monsterinfo, melee),

	SAVE_MEMBER(monsterinfo, dodge),

	SAVE_MEMBER(monsterinfo, sight),
	SAVE_MEMBER(monsterinfo, checkattack),

	SAVE_MEMBER(monsterinfo, pause_framenum),
	SAVE_MEMBER(monsterinfo, attack_finished),

	SAVE_MEMBER(monsterinfo, saved_goal),
	SAVE_MEMBER(monsterinfo, search_framenum),
	SAVE_MEMBER(monsterinfo, trail_framenum),
	SAVE_MEMBER(monsterinfo, last_sighting),
	SAVE_MEMBER(monsterinfo, attack_state),
	SAVE_MEMBER(monsterinfo, lefty),
	SAVE_MEMBER(monsterinfo, idle_framenum),
	SAVE_MEMBER(monsterinfo, linkcount),

	SAVE_MEMBER(monsterinfo, power_armor_type),
	SAVE_MEMBER(monsterinfo, power_armor_power),

#ifdef ROGUE_AI
	SAVE_MEMBER(monsterinfo, blocked),
	SAVE_MEMBER(monsterinfo, last_hint_framenum),
	SAVE_MEMBER(monsterinfo, goal_hint),
	SAVE_MEMBER(monsterinfo, medicTries),
	SAVE_MEMBER(monsterinfo, badMedic1),
	SAVE_MEMBER(monsterinfo, badMedic2),
	SAVE_MEMBER(monsterinfo, healer),
	SAVE_MEMBER(monsterinfo, duck),
	SAVE_MEMBER(monsterinfo, unduck),
	SAVE_MEMBER(monsterinfo, sidestep),
	SAVE_MEMBER(monsterinfo, base_height),
	SAVE_MEMBER(monsterinfo, next_duck_framenum),
	SAVE_MEMBER(monsterinfo, duck_wait_framenum),
	SAVE_MEMBER(monsterinfo, last_player_enemy),
	SAVE_MEMBER(monsterinfo, blindfire),
	SAVE_MEMBER(monsterinfo, blind_fire_framedelay),
	SAVE_MEMBER(monsterinfo, blind_fire_target),
#endif
#ifdef GROUND_ZERO
	SAVE_MEMBER(monsterinfo, monster_slots),
	SAVE_MEMBER(monsterinfo, monster_used),
	SAVE_MEMBER(monsterinfo, commander),
	SAVE_MEMBER(monsterinfo, summon_type),
	SAVE_MEMBER(monsterinfo, quad_framenum),
	SAVE_MEMBER(monsterinfo, invincible_framenum),
	SAVE_MEMBER(monsterinfo, double_framenum)
#endif
};

CREATE_STRUCTURE_SERIALIZE_FUNCS(monsterinfo);
#endif

static save_member entity_members[] = {
	SAVE_MEMBER(entity, s),
	SAVE_MEMBER(entity, linkcount),

	SAVE_MEMBER(entity, svflags),
	SAVE_MEMBER(entity, bounds),
	// absmin, absmax and size skipped intentionally
	SAVE_MEMBER(entity, solid),
	SAVE_MEMBER(entity, clipmask),
	SAVE_MEMBER(entity, owner),
	
	SAVE_MEMBER(entity, movetype),

	SAVE_MEMBER(entity, flags),
	
	SAVE_MEMBER(entity, model),
	SAVE_MEMBER(entity, freeframenum),

	SAVE_MEMBER(entity, message),
	SAVE_MEMBER(entity, type),
	SAVE_MEMBER(entity, spawnflags),
	
	SAVE_MEMBER(entity, timestamp),
	SAVE_MEMBER(entity, angle),
	SAVE_MEMBER(entity, target),
	SAVE_MEMBER(entity, targetname),
	SAVE_MEMBER(entity, killtarget),
	SAVE_MEMBER(entity, team),
	SAVE_MEMBER(entity, pathtarget),
	SAVE_MEMBER(entity, deathtarget),
	SAVE_MEMBER(entity, combattarget),
	SAVE_MEMBER(entity, target_ent),
	
	SAVE_MEMBER(entity, speed),
	SAVE_MEMBER(entity, accel),
	SAVE_MEMBER(entity, decel),
	SAVE_MEMBER(entity, movedir),
	SAVE_MEMBER(entity, pos1),
	SAVE_MEMBER(entity, pos2),
	
	SAVE_MEMBER(entity, velocity),
	SAVE_MEMBER(entity, avelocity),
	SAVE_MEMBER(entity, mass),
	SAVE_MEMBER(entity, air_finished_framenum),
	SAVE_MEMBER(entity, gravity),
	
	SAVE_MEMBER(entity, goalentity),
	SAVE_MEMBER(entity, movetarget),
	SAVE_MEMBER(entity, yaw_speed),
	SAVE_MEMBER(entity, ideal_yaw),
	
	SAVE_MEMBER(entity, nextthink),
	SAVE_MEMBER(entity, prethink),
	SAVE_MEMBER(entity, think),
	
	SAVE_MEMBER(entity, blocked),
	SAVE_MEMBER(entity, touch),
	SAVE_MEMBER(entity, use),
	SAVE_MEMBER(entity, pain),
	SAVE_MEMBER(entity, die),
	
	SAVE_MEMBER(entity, touch_debounce_framenum),
	SAVE_MEMBER(entity, pain_debounce_framenum),
	SAVE_MEMBER(entity, damage_debounce_framenum),
	SAVE_MEMBER(entity, fly_sound_debounce_framenum),
	SAVE_MEMBER(entity, last_move_framenum),
	
	SAVE_MEMBER(entity, health),
	SAVE_MEMBER(entity, max_health),
	SAVE_MEMBER(entity, gib_health),

	SAVE_MEMBER(entity, deadflag),

	SAVE_MEMBER(entity, show_hostile),

	SAVE_MEMBER(entity, powerarmor_framenum),
	
	SAVE_MEMBER(entity, map),
	SAVE_MEMBER(entity, viewheight),
	SAVE_MEMBER(entity, takedamage),
	SAVE_MEMBER(entity, dmg),
	SAVE_MEMBER(entity, radius_dmg),
	SAVE_MEMBER(entity, dmg_radius),
	SAVE_MEMBER(entity, sounds),
	SAVE_MEMBER(entity, count),
	
	SAVE_MEMBER(entity, chain),
	SAVE_MEMBER(entity, enemy),
	SAVE_MEMBER(entity, oldenemy),
	SAVE_MEMBER(entity, activator),
	SAVE_MEMBER(entity, groundentity),
	SAVE_MEMBER(entity, groundentity_linkcount),
	SAVE_MEMBER(entity, teamchain),
	SAVE_MEMBER(entity, teammaster),

	SAVE_MEMBER(entity, mynoise),
	SAVE_MEMBER(entity, mynoise2),

	SAVE_MEMBER(entity, noise_index),
	SAVE_MEMBER(entity, noise_index2),
	SAVE_MEMBER(entity, volume),
	SAVE_MEMBER(entity, attenuation),
		
	SAVE_MEMBER(entity, wait),
	SAVE_MEMBER(entity, delay),
	SAVE_MEMBER(entity, rand),
		
	SAVE_MEMBER(entity, last_sound_framenum),

	SAVE_MEMBER(entity, watertype),
	SAVE_MEMBER(entity, waterlevel),
		
	SAVE_MEMBER(entity, move_origin),
	SAVE_MEMBER(entity, move_angles),
		
	SAVE_MEMBER(entity, style),
	SAVE_MEMBER(entity, item),

#ifdef ROGUE_AI
	SAVE_MEMBER(entity, gravityVector),
	SAVE_MEMBER(entity, bad_area),
	SAVE_MEMBER(entity, hint_chain),
	SAVE_MEMBER(entity, monster_hint_chain),
	SAVE_MEMBER(entity, target_hint_chain),
	SAVE_MEMBER(entity, hint_chain_id),
#endif

#ifdef GROUND_ZERO
	SAVE_MEMBER(entity, plat2flags),
	SAVE_MEMBER(entity, offset),
	SAVE_MEMBER(entity, last_move_framenum),
#endif
		
	SAVE_MEMBER(entity, moveinfo),
	
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(entity, monsterinfo),
#endif
};

DEFINE_SAVE_STRUCTURE(entity);

#ifdef JSON_SAVE_FORMAT
inline void WriteGameStream(stringlit filename)
{
	serializer stream(filename, false);

	cJSON_AddStringToObject(stream.json, "date", __DATE__);

	cJSON_AddItemToObject(stream.json, "game_locals", stream.write_struct(game_locals_save, &game));

	cJSON *clients = cJSON_CreateArray();

	for (auto &e : entity_range(1, game.maxclients))
		cJSON_AddItemToArray(clients, stream.write_struct(client_save, e.client));

	cJSON_AddItemToObject(stream.json, "clients", clients);
}
#else
inline void WriteGameStream(stringlit filename)
{
	serializer stream(filename, false);

	stream << stringref(__DATE__);

	stream.write_struct(game_locals_save, &game);

	for (auto &e : entity_range(1, game.maxclients))
		stream.write_struct(client_save, e.client);
}
#endif

void WriteGame(stringlit filename, qboolean autosave [[maybe_unused]])
{
#ifdef SINGLE_PLAYER
	if (!autosave)
		SaveClientData();

	game.autosaved = autosave;
#endif

	WriteGameStream(filename);

#ifdef SINGLE_PLAYER
	game.autosaved = false;
#endif
}

#ifdef JSON_SAVE_FORMAT
inline void ReadGameStream(stringlit filename)
{
	serializer stream(filename, true);

	WipeEntities();

	stream.read_struct(cJSON_GetObjectItem(stream.json, "game_locals"), game_locals_save, &game);

	cJSON *clients = cJSON_GetObjectItem(stream.json, "clients");

	for (auto &e : entity_range(1, game.maxclients))
		stream.read_struct(cJSON_GetArrayItem(clients, e.s.number - 1), client_save, e.client);
}
#else
inline void ReadGameStream(stringlit filename)
{
	serializer stream(filename, true);
	string date;

	stream >> date;

	if (date != __DATE__)
		gi.error("Savegame from an older version.\n");

	WipeEntities();

	stream.read_struct(game_locals_save, &game);

	for (auto &e : entity_range(1, game.maxclients))
		stream.read_struct(client_save, e.client);
}
#endif

void ReadGame(stringlit filename)
{
	ReadGameStream(filename);
}

#ifdef JSON_SAVE_FORMAT
inline void WriteLevelStream(stringlit filename)
{
	serializer stream(filename, false);

	cJSON_AddNumberToObject(stream.json, "entity_size", sizeof(entity));

	// write out level_locals_t
	cJSON_AddItemToObject(stream.json, "level_locals", stream.write_struct(level_locals_save, &level));

	cJSON *entities_obj = cJSON_CreateObject();

	for (auto &ent : entity_range(0, num_entities - 1))
		if (ent.inuse)
			cJSON_AddItemToObject(entities_obj, itos(ent.s.number).ptr(), stream.write_struct(entity_save, &ent));

	cJSON_AddItemToObject(stream.json, "entities", entities_obj);
}
#else
inline void WriteLevelStream(stringlit filename)
{
	serializer stream(filename, false);

	stream << sizeof(entity);

	// write out level_locals_t
	stream.write_struct(level_locals_save, &level);

	for (auto &ent : entity_range(0, num_entities - 1))
	{
		if (!ent.inuse)
			continue;

		stream << ent.s.number;
		stream.write_struct(entity_save, &ent);
	}

	stream << (uint32_t) -1;
}
#endif

void WriteLevel(stringlit filename)
{
	WriteLevelStream(filename);
}

#ifdef JSON_SAVE_FORMAT
inline uint32_t ReadLevelStream(stringlit filename)
{
	serializer stream(filename, true);

	// load the level locals
	stream.read_struct(cJSON_GetObjectItem(stream.json, "level_locals"), level_locals_save, &level);

	uint32_t file_edicts = game.maxclients;

	cJSON *entities_obj = cJSON_GetObjectItem(stream.json, "entities");

	for (cJSON *cent = entities_obj->child; cent; cent = cent->next)
	{
		uint32_t entnum = atoi(cent->string);

		if ((uint32_t) entnum >= file_edicts)
			file_edicts = entnum + 1;

		entity &ent = itoe(entnum);

		ent.__init();

		ent.inuse = true;

		stream.read_struct(cent, entity_save, &ent);

		// let the server rebuild world links for this ent
		gi.linkentity(ent);
	}

	return file_edicts;
}
#else
inline uint32_t ReadLevelStream(stringlit filename)
{
	serializer stream(filename, true);
	size_t entity_size;

	stream >> entity_size;

	if (entity_size != sizeof(entity))
		gi.error("ReadLevel: mismatched edict size");

	// load the level locals
	stream.read_struct(level_locals_save, &level);

	uint32_t file_edicts = game.maxclients;

	while (true)
	{
		uint32_t id;

		stream >> id;

		if (id == (uint32_t) -1)
			break;

		if ((uint32_t) id >= file_edicts)
			file_edicts = id + 1;

		entity &ent = itoe(id);

		ent.__init();

		ent.inuse = true;

		stream.read_struct(entity_save, &ent);

		// let the server rebuild world links for this ent
		gi.linkentity(ent);
	}

	return file_edicts;
}
#endif

void ReadLevel(stringlit filename)
{
	WipeEntities();

	num_entities = ReadLevelStream(filename);

#ifdef SINGLE_PLAYER
	// mark all clients as unconnected
	for (entity &ent : entity_range(1, game.maxclients))
		ent.client->pers.connected = false;

	// do any load time things at this point
	for (uint32_t i = 0; i < num_entities; i++)
	{
		entity &ent = itoe(i);

		if (!ent.inuse)
			continue;

		// fire any cross-level triggers
		if (ent.type == ET_TARGET_CROSSLEVEL_TARGET)
			ent.nextthink = level.framenum + (gtime) (ent.delay * BASE_FRAMERATE);
	}
#endif
}
#endif
#include "lib/types.h"
#include "lib/gi.h"
#include "lib/server_entity.h"
#include "lib/info.h"
#include "game/player.h"
#include "game/game.h"
#include "game/cmds.h"
#include "game/svcmds.h"
#include "game/spawn.h"
#include "game/itemlist.h"

#include <fstream>

#define CJSON_HIDE_SYMBOLS
#include "debug/cJSON.h"

struct binary_serializer;
struct json_serializer;

using serialize_write_func = void(*)(binary_serializer &, const void *, const void *);
using serialize_read_func = void(*)(binary_serializer &, void *, void *);
using json_write_func = cJSON *(*)(json_serializer &, const void *, const void *);
using json_read_func = void(*)(cJSON *, json_serializer &, void *, void *);

struct save_member
{
	const stringlit			name;
	const size_t			offset, size;
	serialize_write_func	write;
	serialize_read_func		read;
	json_write_func			write_json;
	json_read_func			read_json;
};

struct save_struct
{
	const stringlit		name;
	const save_member	*members;
	const size_t		num_members;
};

struct binary_serializer
{
	// The following types can be written/read directly without any extra work.
	template<typename T>
	static constexpr bool is_trivial = !std::is_pointer_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T> ||
		std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat> ||
		std::is_same_v<T, vector> || std::is_same_v<T, pmove_state> || std::is_same_v<T, player_state> || std::is_same_v<T, entity_state>);

	std::iostream	&stream;

	binary_serializer(std::iostream &stream) :
		stream(stream)
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
			member->write(*this, (uint8_t *)ptr + member->offset, ptr);
	}

	void read_struct(const save_struct &struc, void *ptr)
	{
		for (auto member = struc.members; member != struc.members + struc.num_members; member++)
			member->read(*this, (uint8_t *)ptr + member->offset, ptr);
	}
};

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
			cJSON* obj = member->write_json(*this, (uint8_t*)ptr + member->offset, ptr);

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
			for (auto member = struc.members; member != struc.members + struc.num_members; member++)
				member->read_json(child, *this, (uint8_t *)ptr + member->offset, ptr);
	}
};

// The following types can be serialized to JSON as numbers
template<typename T>
static constexpr bool is_number = !std::is_pointer_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>);
	
template<typename T> requires is_number<T>
inline cJSON *json_serializer_write(json_serializer&, const T &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateNumber((double) v);
}

template<typename T> requires std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat>
inline cJSON *json_serializer_write(json_serializer&, const T &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateNumber((double) (int32_t) v);
}

inline cJSON *json_serializer_write(json_serializer&, const bool &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateTrue();
}

inline cJSON *json_serializer_write(json_serializer&, const stringref &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v.ptr());
}

inline cJSON *json_serializer_write(json_serializer&, const string &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateString(v.ptr());
}

inline cJSON *json_serializer_write(json_serializer&, const entityref &v, const bool& defaultable = false)
{
	if (defaultable && !v.has_value())
		return nullptr;

	return cJSON_CreateNumber((double)v->s.number);
}

inline cJSON *json_serializer_write(json_serializer&, const itemref &v, const bool& defaultable = false)
{
	if (defaultable && !v.has_value())
		return nullptr;

	return cJSON_CreateNumber((double) v->id);
}

inline cJSON *json_serializer_write(json_serializer&, const vector &v, const bool& defaultable = false)
{
	if (defaultable && !v)
		return nullptr;

	return cJSON_CreateFloatArray(&v.x, 3);
}

template<typename TFunc>
inline cJSON *json_serializer_write(json_serializer&, const savable_function<TFunc> &str, const bool& defaultable = false)
{
	if (str)
		return cJSON_CreateString(str.func->name);

	return defaultable ? nullptr : cJSON_CreateStringReference("");
}

template<typename T>
inline cJSON *json_serializer_write(json_serializer&, const savable_data<T> &str, const bool& defaultable = false)
{
	if (str)
		return cJSON_CreateString(str.data->name);

	return defaultable ? nullptr : cJSON_CreateStringReference("");
}
	
template<typename T, size_t N>
inline cJSON *json_serializer_write(json_serializer &stream, const array<T, N> &arr, const bool &defaultable = false)
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
inline void json_serializer_read(cJSON *json, json_serializer &, T &str)
{
	str = (T) cJSON_GetNumberValue(json);
}

template<typename T> requires std::is_same_v<T, sound_index> || std::is_same_v<T, image_index> || std::is_same_v<T, model_index> || std::is_same_v<T, player_stat>
inline void json_serializer_read(cJSON *json, json_serializer &, T &str)
{
	str = (T) (int32_t) cJSON_GetNumberValue(json);
}

inline void json_serializer_read(cJSON *json, json_serializer &, bool &str)
{
	str = cJSON_IsTrue(json) ? true : false;
}

inline void json_serializer_read(cJSON *json, json_serializer &, stringref &str)
{
	str = cJSON_GetStringValue(json);
}

inline void json_serializer_read(cJSON *json, json_serializer &, string &str)
{
	str = cJSON_GetStringValue(json);
}

inline void json_serializer_read(cJSON *json, json_serializer &, entityref &str)
{
	if (cJSON_IsNull(json))
		str = entityref();
	else
		str = itoe((size_t) cJSON_GetNumberValue(json));
}

inline void json_serializer_read(cJSON *json, json_serializer &, itemref &str)
{
	str = GetItemByIndex((gitem_id) cJSON_GetNumberValue(json));
}

inline void json_serializer_read(cJSON *json, json_serializer &, vector &str)
{
	for (int32_t i = 0; i < 3; i++)
		str[i] = (float) cJSON_GetNumberValue(cJSON_GetArrayItem(json, i));
}

template<typename T, size_t N>
inline void json_serializer_read(cJSON *json, json_serializer &stream, array<T, N> &arr)
{
	int i = 0;

	for (auto &v : arr)
		json_serializer_read(cJSON_GetArrayItem(json, i++), stream, v);
}

template<typename TFunc>
inline void json_serializer_read(cJSON *json, json_serializer &stream, savable_function<TFunc> &str)
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
inline void json_serializer_read(cJSON *json, json_serializer &stream, savable_data<T> &str)
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

#define SAVE_MEMBER(type, member) \
	{ #member, offsetof(type, member), sizeof(type::member), \
		[](binary_serializer &stream, const void *data, const void *) { stream << *(decltype(type::member) *) data; }, \
		[](binary_serializer &stream, void *data, void *) { stream >> *(decltype(type::member) *) data; }, \
		[](json_serializer &stream, const void *data, const void *) { return json_serializer_write(stream, *(decltype(type::member) *) data, true); }, \
		[](cJSON *json, json_serializer &stream, void *data, void *) { json_serializer_read(json, stream, *(decltype(type::member) *) data); } }

// A few things in our code rely on getters/setters to hide
// protocol-based behavior.
#define SAVE_MEMBER_PROPERTY(type, member) \
	{ #member, 0, sizeof(std::invoke_result_t<decltype(&type::get_##member), type>), \
		[](binary_serializer &stream, const void *, const void *struc) { stream << ((type *) struc)->get_##member##(); }, \
		[](binary_serializer &stream, void *, void *struc) { std::invoke_result_t<decltype(&type::get_##member), type> temp; stream >> temp; ((type *) struc)->set_##member##(temp); }, \
		[](json_serializer &stream, const void *, const void *struc) { return json_serializer_write(stream, ((type *) struc)->get_##member##(), true); }, \
		[](cJSON *json, json_serializer &stream, void *, void *struc) { std::invoke_result_t<decltype(&type::get_##member), type> temp; json_serializer_read(json, stream, temp); ((type *) struc)->set_##member##(temp); } }

#define DEFINE_SAVE_STRUCTURE(struct_name) \
	static save_struct struct_name##_save = { \
		#struct_name, \
		struct_name##_members, \
		lengthof(struct_name##_members) \
	}

#define CREATE_STRUCTURE_SERIALIZE_FUNCS(type) \
	DEFINE_SAVE_STRUCTURE(type); \
	inline void operator<<(binary_serializer &stream, const type &str) { stream.write_struct(type##_save, &str); } \
	inline void operator>>(binary_serializer &stream, type &str) { stream.read_struct(type##_save, &str); } \
	inline cJSON *json_serializer_write(json_serializer &stream, const type &str, const bool &) { return stream.write_struct(type##_save, &str, true); } \
	inline void json_serializer_read(cJSON *json, json_serializer &stream, type &str) { stream.read_struct(json, type##_save, &str); }

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

static save_member entity_members[] = {
	SAVE_MEMBER(entity, s),
	SAVE_MEMBER(entity, linkcount),

	SAVE_MEMBER(entity, svflags),
	SAVE_MEMBER(entity, mins),
	SAVE_MEMBER(entity, maxs),
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

#ifdef GROUND_ZERO
	SAVE_MEMBER(entity, plat2flags),
	SAVE_MEMBER(entity, offset),
	SAVE_MEMBER(entity, gravityVector),
	SAVE_MEMBER(entity, bad_area),
	SAVE_MEMBER(entity, hint_chain),
	SAVE_MEMBER(entity, monster_hint_chain),
	SAVE_MEMBER(entity, target_hint_chain),
	SAVE_MEMBER(entity, hint_chain_id),
	SAVE_MEMBER(entity, lastMoveFrameNum),
#endif
		
	SAVE_MEMBER(entity, moveinfo.start_origin),
	SAVE_MEMBER(entity, moveinfo.start_angles),
	SAVE_MEMBER(entity, moveinfo.end_origin),
	SAVE_MEMBER(entity, moveinfo.end_angles),

	SAVE_MEMBER(entity, moveinfo.sound_start),
	SAVE_MEMBER(entity, moveinfo.sound_middle),
	SAVE_MEMBER(entity, moveinfo.sound_end),

	SAVE_MEMBER(entity, moveinfo.accel),
	SAVE_MEMBER(entity, moveinfo.speed),
	SAVE_MEMBER(entity, moveinfo.decel),
	SAVE_MEMBER(entity, moveinfo.distance),

	SAVE_MEMBER(entity, moveinfo.wait),
		
	SAVE_MEMBER(entity, moveinfo.state),
	SAVE_MEMBER(entity, moveinfo.dir),
	SAVE_MEMBER(entity, moveinfo.current_speed),
	SAVE_MEMBER(entity, moveinfo.move_speed),
	SAVE_MEMBER(entity, moveinfo.next_speed),
	SAVE_MEMBER(entity, moveinfo.remaining_distance),
	SAVE_MEMBER(entity, moveinfo.decel_distance),

	SAVE_MEMBER(entity, moveinfo.endfunc),
	
#ifdef SINGLE_PLAYER
	SAVE_MEMBER(entity, monsterinfo.currentmove),
	SAVE_MEMBER(entity, monsterinfo.aiflags),
	SAVE_MEMBER(entity, monsterinfo.nextframe),
	SAVE_MEMBER(entity, monsterinfo.scale),
		
	SAVE_MEMBER(entity, monsterinfo.stand),
	SAVE_MEMBER(entity, monsterinfo.idle),
	SAVE_MEMBER(entity, monsterinfo.search),
	SAVE_MEMBER(entity, monsterinfo.walk),
	SAVE_MEMBER(entity, monsterinfo.run),
	SAVE_MEMBER(entity, monsterinfo.dodge),
	SAVE_MEMBER(entity, monsterinfo.attack),
	SAVE_MEMBER(entity, monsterinfo.melee),
	SAVE_MEMBER(entity, monsterinfo.sight),
	SAVE_MEMBER(entity, monsterinfo.checkattack),
		
	SAVE_MEMBER(entity, monsterinfo.pause_framenum),
	SAVE_MEMBER(entity, monsterinfo.attack_finished),
		
	SAVE_MEMBER(entity, monsterinfo.saved_goal),
	SAVE_MEMBER(entity, monsterinfo.search_framenum),
	SAVE_MEMBER(entity, monsterinfo.trail_framenum),
	SAVE_MEMBER(entity, monsterinfo.last_sighting),
	SAVE_MEMBER(entity, monsterinfo.attack_state),
	SAVE_MEMBER(entity, monsterinfo.lefty),
	SAVE_MEMBER(entity, monsterinfo.idle_framenum),
	SAVE_MEMBER(entity, monsterinfo.linkcount),

	SAVE_MEMBER(entity, monsterinfo.power_armor_type),
	SAVE_MEMBER(entity, monsterinfo.power_armor_power),
#endif
};

DEFINE_SAVE_STRUCTURE(entity);

void WipeEntities();
static inline void PostReadLevel(uint32_t num_edicts [[maybe_unused]]);

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
		json_serializer stream(filename, false);

		if (!autosave)
			SaveClientData ();

		cJSON_AddStringToObject(stream.json, "date", __DATE__);

		game.autosaved = autosave;

		cJSON_AddItemToObject(stream.json, "game_locals", stream.write_struct(game_locals_save, &game));

		game.autosaved = false;

		cJSON *clients = cJSON_CreateArray();

		for (auto &e : entity_range(1, game.maxclients))
			cJSON_AddItemToArray(clients, stream.write_struct(client_save, e.client));

		cJSON_AddItemToObject(stream.json, "clients", clients);
	};
	
	void (*ReadGame)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
		std::fstream f(filename, std::fstream::in | std::fstream::binary);

		if (!f.is_open())
			gi.error ("Couldn't open %s", filename);

		stringarray<16> str;
		f.read(str.data(), 16);

		if (str != __DATE__)
		{
			f.close();
			gi.error ("Savegame from an older version.\n");
		}

		WipeEntities();

		binary_serializer stream(f);

		stream.read_struct(game_locals_save, &game);
		
		for (auto &e : entity_range(1, game.maxclients))
			stream.read_struct(client_save, e.client);
	};

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void (*WriteLevel)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
		json_serializer stream(filename, false);

		cJSON_AddNumberToObject(stream.json, "entity_size", sizeof(entity));

		// write out level_locals_t
		cJSON_AddItemToObject(stream.json, "level_locals", stream.write_struct(level_locals_save, &level));

		cJSON *entities_obj = cJSON_CreateObject();

		for (auto &ent : entity_range(0, num_entities - 1))
			if (ent.inuse)
				cJSON_AddItemToObject(entities_obj, itos(ent.s.number).ptr(), stream.write_struct(entity_save, &ent));

		cJSON_AddItemToObject(stream.json, "entities", entities_obj);
	};
	
	void (*ReadLevel)(stringlit filename) = [](stringlit filename [[maybe_unused]])
	{
		std::fstream f(filename, std::fstream::in | std::fstream::binary);

		if (!f.is_open())
			gi.error ("Couldn't open %s", filename);

		WipeEntities();

		binary_serializer stream(f);

		size_t i;
		stream >> i;

		if (i != sizeof(entity))
		{
			f.close();
			gi.error ("ReadLevel: mismatched edict size");
		}
		
		// load the level locals
		stream.read_struct(level_locals_save, &level);

		uint32_t file_edicts = game.maxclients;

		// load all the entities
		while (1)
		{
			int32_t entnum;
			stream >> entnum;

			if (!f.gcount())
			{
				f.close();
				gi.error ("ReadLevel: failed to read entnum");
			}
			if (entnum == -1)
				break;
			if ((uint32_t) entnum >= file_edicts)
				file_edicts = entnum + 1;

			entity &ent = itoe(entnum);

			stream.read_struct(entity_save, &ent);

			ent.inuse = true;

			// let the server rebuild world links for this ent
			gi.linkentity (ent);
		}

		f.close();

		PostReadLevel(file_edicts);
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
	this->s.number = (uint32_t) (this - ge.edicts);
	this->client = cl;
}

void entity::__free()
{
	::client *cl = client;
	this->~entity();
	memset(this, 0, sizeof(*this));
	this->s.number = (uint32_t) (this - ge.edicts);
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

void WipeEntities()
{
	for (auto &e : entity_range(0, max_entities - 1))
		if (e.inuse)
			e.__free();
}

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
	ge.num_edicts = num_edicts;
	
	// mark all clients as unconnected
	for (uint32_t i = 0 ; i < game.maxclients; i++)
	{
		entity &ent = itoe(i + 1);
		ent.client->pers.connected = false;
	}

	// do any load time things at this point
	for (uint32_t i = 0 ; i < ge.num_edicts; i++)
	{
		entity &ent = itoe(i);

		if (!ent.inuse)
			continue;

		// fire any cross-level triggers
		if (ent.type == ET_TARGET_CROSSLEVEL_TARGET)
			ent.nextthink = level.framenum + (gtime)(ent.delay * BASE_FRAMERATE);
	}
#endif
}

extern "C" game_export *GetGameAPI(game_import_impl *impl)
{
	gi.set_impl(impl);

	ge.edicts = gi.TagMalloc<entity>(ge.max_edicts, TAG_GAME);

	// s.number must always be assigned
	for (auto &e : entity_range(0, max_entities - 1))
		e.s.number = (int) (&e - ge.edicts);

	world = ge.edicts[0];

	return &ge;
}
#pragma once

#include "config.h"
#include "lib/string.h"
#include "lib/protocol.h"
#include "lib/types/dynarray.h"
#include "lib/math/vector.h"
#include "lib/math/bbox.h"
#include "lib/types.h"
#include "game/entity_types.h"
#include "lib/string/format.h"

// impl passed from engine.
// this type should not be modified, but needs to be here for template resolution below.
struct game_import_impl
{
	// special messages
	void (*bprintf)(int32_t printlevel, const char *fmt, ...);
	void (*dprintf)(const char *fmt, ...);
	void (*cprintf)(entity *ent, int32_t printlevel, const char *fmt, ...);
	void (*centerprintf)(entity *ent, const char *fmt, ...);
	void (*sound)(entity *ent, int32_t channel, int soundindex, float volume, float attenuation, float time_offset);
	void (*positioned_sound)(const float *origin, entity *ent, int32_t channel, int32_t soundindex, float volume, float attenuation, float time_offset);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void (*configstring)(int32_t num, const char *string);

	void (*error)(const char *fmt, ...);

	// the *index functions create configstrings and some internal server state
	int (*modelindex)(const char *name);
	int (*soundindex)(const char *name);
	int (*imageindex)(const char *name);

	void (*setmodel)(entity *ent, const char *name);

	// collision detection
	trace(*trace)(const float *start, const float *mins, const float *maxs, const float *end, entity *passent, int32_t contentmask);
	int32_t(*pointcontents)(const float *point);
	qboolean(*inPVS)(const float *p1, const float *p2);
	qboolean(*inPHS)(const float *p1, const float *p2);
	void (*SetAreaPortalState)(int portalnum, qboolean open);
	qboolean(*AreasConnected)(int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void (*linkentity)(entity *ent);
	void (*unlinkentity)(entity *ent);     // call before removing an interactive edict
	int (*BoxEdicts)(const float *mins, const float *maxs, entity **list, int maxcount, int32_t areatype);
	void (*Pmove)(pmove *pmove);          // player movement code common with client prediction

											// network messaging
	void (*multicast)(const float *origin, int32_t to);
	void (*unicast)(entity *ent, qboolean reliable);
	void (*WriteChar)(int c);
	void (*WriteByte)(int c);
	void (*WriteShort)(int c);
	void (*WriteLong)(int c);
	void (*WriteFloat)(float f);
	void (*WriteString)(const char *s);
	void (*WritePosition)(const float *pos);    // some fractional bits
	void (*WriteDir)(const float *pos);         // single byte encoded, very coarse
	void (*WriteAngle)(float f);

	// managed memory allocation
	void *(*TagMalloc)(uint32_t size, uint32_t tag);
	void (*TagFree)(void *block);
	void (*FreeTags)(uint32_t tag);

	// console variable interaction
	cvar *(*cvar)(const char *var_name, const char *value, int32_t flags);
	::cvar *(*cvar_set)(const char *var_name, const char *value);
	::cvar *(*cvar_forceset)(const char *var_name, const char *value);

	// ClientCommand and ServerCommand parameter access
	int (*argc)(void);
	char *(*argv)(int n);
	char *(*args)(void);     // concatenation of all argv >= 1

							 // add commands to the server console as if they were typed in
							 // for map changing, etc
	void (*AddCommandString)(const char *text);

	void (*DebugGraph)(float value, int color);

	// Knightmare- support game DLL loading from pak files thru engine
	// This can be used to load script files, etc
#ifdef KMQUAKE2_ENGINE_MOD
	char **(*ListPak) (char *find, int *num);	// Deprecated- DO NOT USE!
	int		(*LoadFile) (char *name, void **buf);
	void	(*FreeFile) (void *buf);
	void	(*FreeFileList) (char **list, int n);
	int		(*OpenFile) (const char *name, fileHandle_t *f, fsMode_t mode);
	int		(*OpenCompressedFile) (const char *zipName, const char *fileName, fileHandle_t *f, fsMode_t mode);
	void	(*CloseFile) (fileHandle_t f);
	int		(*FRead) (void *buffer, int size, fileHandle_t f);
	int		(*FWrite) (const void *buffer, int size, fileHandle_t f);
	char *(*FS_GameDir) (void);
	char *(*FS_SaveGameDir) (void);
	void	(*CreatePath) (char *path);
	char **(*GetFileList) (const char *path, const char *extension, int *num);
#endif
};

// a special wrapper type for a "dir", which is written
// differently to the network.
struct vecdir : vector
{
};

// a special type for an "angle", which is written
// differently to the network.
struct anglef
{
	float angle;
};

struct game_import
{
private:
	game_import_impl impl;

public:
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

	template<typename T, typename ...Args>
	[[nodiscard]] inline T *TagNew(const uint32_t &tag, const Args &...args)
	{
		auto ptr = (T *) TagMalloc(sizeof(T), tag);
		::new(ptr) T(std::forward<const Args>(args)...);
		return ptr;
	}

	[[nodiscard]] void *TagMalloc(uint32_t size, uint32_t tag);

	void TagFree(void *block);

	template<typename T>
	inline void TagDelete(T *obj)
	{
		obj->~T();
		TagFree(obj);
	}

	void FreeTags(uint32_t tag);

	// chat functions

	// broadcast print; prints to all connected clients + console
	// with C-like formatting
	inline void bprint(print_level printlevel, const stringref &str)
	{
		impl.bprintf(printlevel, "%s", str.ptr());
	}

	// broadcast print, but with string formatter
	template<typename ...Args>
	inline void bprintfmt(print_level printlevel, stringlit fmt, const Args &...args)
	{
		impl.bprintf(printlevel, "%s", format(fmt, std::forward<const Args>(args)...).data());
	}

	// debug print; prints only to console or listen server host
	inline void dprint(const stringref &str)
	{
		impl.dprintf("%s", str.ptr());
	}

	// debug print, but with string formatter
	template<typename ...Args>
	inline void dprintfmt(stringlit fmt, const Args &...args)
	{
		impl.dprintf("%s", format(fmt, std::forward<const Args>(args)...).data());
	}

	// client print; prints to one client
	inline void cprint(const entity &ent, print_level printlevel, const stringref &str)
	{
		impl.cprintf(const_cast<entity *>(&ent), printlevel, "%s", str.ptr());
	}

	// client print, but with string formatter
	template<typename ...Args>
	inline void cprintfmt(const entity &ent, print_level printlevel, stringlit fmt, const Args &...args)
	{
		impl.cprintf(const_cast<entity *>(&ent), printlevel, "%s", format(fmt, std::forward<const Args>(args)...).data());
	}

	// center print; prints on center of screen to client
	inline void centerprint(const entity &ent, const stringref &str)
	{
		impl.centerprintf(const_cast<entity *>(&ent), "%s", str.ptr());
	}

	// center print, but with string formatter
	template<typename ...Args>
	inline void centerprintfmt(const entity &ent, stringlit fmt, const Args &...args)
	{
		impl.centerprintf(const_cast<entity *>(&ent), "%s", format(fmt, std::forward<const Args>(args)...).data());
	}

	// sounds

	// fetch a sound index from the specified sound file
	sound_index soundindex(const stringref &name);

	// fetch sound index, but formatted input
	template<typename ...Args>
	inline sound_index soundindexfmt(stringlit fmt, const Args &...args)
	{
		return (sound_index) impl.soundindex(format(fmt, std::forward<const Args>(args)...).data());
	}

	// sound; play soundindex on specified entity on channel at specified volume/attn with a time offset of timeofs
	void sound(const entity &ent, sound_channel channel, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f);

	// short overload that drops auto-channel
	inline void sound(const entity &ent, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f)
	{
		sound(ent, CHAN_AUTO, soundindex, volume, attenuation, time_offset);
	}

	// short overload that drops conflicting floating point defaults
	inline void sound(const entity &ent, sound_channel channel, sound_index soundindex, sound_attn attenuation)
	{
		sound(ent, channel, soundindex, 1.f, attenuation, 0.f);
	}

	// short overload that drops conflicting floating point defaults + auto-channel
	inline void sound(const entity &ent, sound_index soundindex, sound_attn attenuation)
	{
		sound(ent, CHAN_AUTO, soundindex, 1.f, attenuation, 0.f);
	}

	// sound; play soundindex at specified origin (from specified entity) on channel at specified volume/attn with a time offset of timeofs
	void positioned_sound(vector origin, entityref ent, sound_channel channel, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f);

	inline void positioned_sound(vector origin, sound_channel channel, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f)
	{
		positioned_sound(origin, world, channel, soundindex, volume, attenuation, time_offset);
	}

	inline void positioned_sound(vector origin, entityref ent, sound_channel channel, sound_index soundindex, sound_attn attenuation)
	{
		positioned_sound(origin, ent, channel, soundindex, 1.f, attenuation, 0.f);
	}

	inline void positioned_sound(vector origin, entityref ent, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f)
	{
		positioned_sound(origin, ent, CHAN_AUTO, soundindex, volume, attenuation, time_offset);
	}

	inline void positioned_sound(vector origin, sound_index soundindex, float volume = 1.f, sound_attn attenuation = ATTN_NORM, float time_offset = 0.f)
	{
		positioned_sound(origin, world, CHAN_AUTO, soundindex, volume, attenuation, time_offset);
	}

	inline void positioned_sound(vector origin, sound_channel channel, sound_index soundindex, sound_attn attenuation)
	{
		positioned_sound(origin, world, channel, soundindex, 1.f, attenuation, 0.f);
	}

	inline void positioned_sound(vector origin, entityref ent, sound_index soundindex, sound_attn attenuation)
	{
		positioned_sound(origin, ent, CHAN_AUTO, soundindex, 1.f, attenuation, 0.f);
	}

	inline void positioned_sound(vector origin, sound_index soundindex, sound_attn attenuation)
	{
		positioned_sound(origin, world, CHAN_AUTO, soundindex, 1.f, attenuation, 0.f);
	}

	// models

	// fetch a model index from the specified sound file
	model_index modelindex(const stringref &name);
	
	// fetch model index, but formatted input
	template<typename ...Args>
	inline model_index modelindexfmt(stringlit fmt, const Args &...args)
	{
		return (model_index) impl.modelindex(format(fmt, std::forward<const Args>(args)...).data());
	}
	
	// set model to specified string value; use this for bmodels, also sets mins/maxs
	void setmodel(entity &ent, const stringref &name);

	// images

	// fetch a model index from the specified sound file
	image_index imageindex(const stringref &name);

	// fetch image index, but formatted input
	template<typename ...Args>
	inline image_index imageindexfmt(stringlit fmt, const Args &...args)
	{
		return (image_index) impl.imageindex(format(fmt, std::forward<const Args>(args)...).data());
	}

	// entities
	
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void linkentity(entity &ent);
	// call before removing an interactive edict
	void unlinkentity(entity &ent);
	// return entities within the specified box
	dynarray<entityref> BoxEdicts(bbox bounds, box_edicts_area areatype, uint32_t allocate = 16);
	// player movement code common with client prediction
	void Pmove(pmove &pmove);

	// collision
	
	// perform a line trace
	[[nodiscard]] trace traceline(vector start, vector end, entityref passent, content_flags contentmask);
	// perform a box trace
	[[nodiscard]] trace trace(vector start, bbox bounds, vector end, entityref passent, content_flags contentmask);
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
	void WriteEntity(const server_entity &e);
	void WriteLong(int32_t c);
	void WriteFloat(float f);
	void WriteString(stringlit s);
	inline void WriteString(const stringref &s)
	{
		WriteString(s.ptr());
	}
	template<typename ...Args>
	inline void WriteStringFmt(stringlit fmt, const Args &...args)
	{
		WriteString(format(fmt, std::forward<const Args>(args)...).data());
	}
	void WritePosition(vector pos);
	void WriteDir(vector dir);
	void WriteAngle(float f);

	// send the currently-buffered message to all clients within
	// the specified destination
	void multicast(vector origin, multicast_destination to);
	
	// send the currently-buffered message to the specified
	// client.
	void unicast(const entity &ent, bool reliable);

	inline void WriteMessageComponent(const uint8_t &b) { WriteByte(b); }
	inline void WriteMessageComponent(const int8_t &b) { WriteChar(b); }
	inline void WriteMessageComponent(const uint16_t &b) { WriteShort(b); }
	inline void WriteMessageComponent(const int16_t &b) { WriteShort(b); }
	inline void WriteMessageComponent(const uint32_t &b) { WriteLong(b); }
	inline void WriteMessageComponent(const int32_t &b) { WriteLong(b); }
	inline void WriteMessageComponent(const float &f) { WriteFloat(f); }
	inline void WriteMessageComponent(const anglef &f) { WriteAngle(f.angle); }
	inline void WriteMessageComponent(stringlit s) { WriteString(s); }
	inline void WriteMessageComponent(const stringref &s) { WriteString(s.ptr()); }
	inline void WriteMessageComponent(const mutable_string &s) { WriteString(s.data()); }
	inline void WriteMessageComponent(const vector &v) { WritePosition(v); }
	inline void WriteMessageComponent(const vecdir &v) { WriteDir((vector) v); }
	inline void WriteMessageComponent(const server_entity &e) { WriteEntity(e); }
	template<std::invocable T>
	inline void WriteMessageComponent(const T &i) { i(); }

	template<typename ...Args>
	inline void WriteMessageComponents(const Args&... args)
	{
		(WriteMessageComponent(args), ...);
	}

	// packetized message type; you don't usually work directly with this
	// type, it's given to you by ConstructMessage.
	template<std::invocable T>
	struct packetized_message
	{
		const T &write_components;
		game_import &gi;

		// send this message directly to this entity
		packetized_message &unicast(entity &ent, bool reliable)
		{
			write_components();
			gi.unicast(ent, reliable);
			return *this;
		}

		// broadcast this message, originating from the specified position,
		// to the matched destination players
		packetized_message &multicast(vector position, multicast_destination dest)
		{
			write_components();
			gi.multicast(position, dest);
			return *this;
		}
	};

public:
	// Easier method of constructing packetized multicast messages
	template<typename ...Args>
	[[nodiscard]] inline auto ConstructMessage(svc_ops op, const Args&... args)
	{
		return packetized_message { [&] () {
			WriteMessageComponent(op);
			(WriteMessageComponent(args), ...);
		}, *this };
	}

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void configstring(config_string num, const stringref &string);

	// configstring but formatted
	template<typename ...Args>
	inline void configstringfmt(config_string num, stringlit fmt, const Args &...args)
	{
		configstring(num, format(fmt, std::forward<const Args &>(args)...).data());
	}

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

	// formatted versions
	template<typename ...Args>
	inline ::cvar &cvar_forcesetfmt(stringlit var_name, stringlit fmt, const Args &...args)
	{
		return cvar_forceset(var_name, format(fmt, std::forward<const Args &>(args)...).data());
	}

	template<typename ...Args>
	inline ::cvar &cvar_setfmt(stringlit var_name, stringlit fmt, const Args &...args)
	{
		return cvar_set(var_name, format(fmt, std::forward<const Args &>(args)...).data());
	}

	template<typename ...Args>
	inline ::cvar &cvarfmt(stringlit var_name, cvar_flags flags, stringlit fmt, const Args &...args)
	{
		return cvar(var_name, format(fmt, std::forward<const Args &>(args)...).data(), flags);
	}

	// misc

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void AddCommandString(const stringref &text);

	// add command string but formatted
	template<typename ...Args>
	inline void AddCommandStringFmt(stringlit fmt, const Args &...args)
	{
		AddCommandString(format(fmt, std::forward<const Args &>(args)...).data());
	}

	void DebugGraph(float value, int color);

	// drop to console, error
	[[noreturn]] void error(stringlit str)
	{
		impl.error("%s", str);
	}

	template<typename ...Args>
	[[noreturn]] inline void errorfmt(stringlit fmt, const Args &...args)
	{
		error(format(fmt, std::forward<const Args &>(args)...).data());
	}
};

extern game_import gi;
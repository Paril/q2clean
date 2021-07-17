#include "gi.h"
#include "server_entity.h"

game_import gi;

extern "C" struct game_import_impl
{
	// special messages
	void (*bprintf)(int32_t printlevel, const char *fmt, ...);
	void (*dprintf)(const char *fmt, ...);
	void (*cprintf)(entity *ent, int32_t printlevel, const char *fmt, ...);
	void (*centerprintf)(entity *ent, const char *fmt, ...);
	void (*sound)(entity *ent, int32_t channel, int soundindex, float volume, float attenuation, float timeofs);
	void (*positioned_sound)(const float *origin, entity *ent, int32_t channel, int32_t soundindex, float volume, float attenuation, float timeofs);

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
	trace (*trace)(const float *start, const float *mins, const float *maxs, const float *end, entity *passent, int32_t contentmask);
	int32_t (*pointcontents)(const float *point);
	qboolean (*inPVS)(const float *p1, const float *p2);
	qboolean (*inPHS)(const float *p1, const float *p2);
	void (*SetAreaPortalState)(int portalnum, qboolean open);
	qboolean (*AreasConnected)(int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void (*linkentity)(entity *ent);
	void (*unlinkentity)(entity *ent);     // call before removing an interactive edict
	int (*BoxEdicts)(const float *mins, const float *maxs, entity **list, int maxcount, int32_t areatype);
	void (*Pmove)(pmove_t *pmove);          // player movement code common with client prediction

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
	char	**(*ListPak) (char *find, int *num);	// Deprecated- DO NOT USE!
	int		(*LoadFile) (char *name, void **buf);
	void	(*FreeFile) (void *buf);
	void	(*FreeFileList) (char **list, int n);
	int		(*OpenFile) (const char *name, fileHandle_t *f, fsMode_t mode);
	int		(*OpenCompressedFile) (const char *zipName, const char *fileName, fileHandle_t *f, fsMode_t mode);
	void	(*CloseFile) (fileHandle_t f);
	int		(*FRead) (void *buffer, int size, fileHandle_t f);
	int		(*FWrite) (const void *buffer, int size, fileHandle_t f);
	char	*(*FS_GameDir) (void);
	char	*(*FS_SaveGameDir) (void);
	void	(*CreatePath) (char *path);
	char	**(*GetFileList) (const char *path, const char *extension, int *num);
#endif
};

game_import_impl impl;

void game_import::set_impl(game_import_impl *implptr)
{
	impl = *implptr;
}

bool game_import::is_ready()
{
	return impl.AddCommandString;
}

[[nodiscard]] void *game_import::TagMalloc(uint32_t size, uint32_t tag)
{
	return impl.TagMalloc(size, tag);
}

void game_import::TagFree(void *block)
{
	impl.TagFree(block);
}

void game_import::FreeTags(uint32_t tag)
{
	impl.FreeTags(tag);
}

// chat functions
#include <cstdarg>

// broadcast print; prints to all connected clients + console
void game_import::bprintf(print_level printlevel, stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string b = va(fmt, argptr);
	impl.bprintf(printlevel, "%s", b.ptr());
	va_end(argptr);
}

// debug print; prints only to console or listen server host
void game_import::dprintf(stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string b = va(fmt, argptr);
	impl.dprintf("%s", b.ptr());
	va_end(argptr);
}

// client print; prints to one client
void game_import::cprintf(const entity &ent, print_level printlevel, stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string b = va(fmt, argptr);
	impl.cprintf(const_cast<entity *>(&ent), printlevel, "%s", b.ptr());
	va_end(argptr);
}

// center print; prints on center of screen to client
void game_import::centerprintf(const entity &ent, stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string b = va(fmt, argptr);
	impl.centerprintf(const_cast<entity *>(&ent), "%s", b.ptr());
	va_end(argptr);
}

// sounds

// fetch a sound index from the specified sound file
sound_index game_import::soundindex(const stringref &name)
{
	return (sound_index)impl.soundindex(name.ptr());
}

// sound; play soundindex on specified entity on channel at specified volume/attn with a time offset of timeofs
void game_import::sound(const entity &ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float timeofs)
{
	impl.sound(const_cast<entity *>(&ent), channel, (int32_t)soundindex, volume, attenuation, timeofs);
}
	
// sound; play soundindex at specified origin (from specified entity) on channel at specified volume/attn with a time offset of timeofs
void game_import::positioned_sound(vector origin, entityref ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float timeofs)
{
	impl.positioned_sound(&origin.x, ent, channel, (int32_t)soundindex, volume, attenuation, timeofs);
}

// models

// fetch a model index from the specified sound file
model_index game_import::modelindex(const stringref &name)
{
	return (model_index)impl.modelindex(name.ptr());
}
	
// set model to specified string value; use this for bmodels, also sets mins/maxs
void game_import::setmodel(entity &ent, const stringref &name)
{
	impl.setmodel(&ent, name.ptr());
}

// images

// fetch a model index from the specified sound file
image_index game_import::imageindex(const stringref &name)
{
	return (image_index)impl.imageindex(name.ptr());
}

// entities
	
// an entity will never be sent to a client or used for collision
// if it is not passed to linkentity.  If the size, position, or
// solidity changes, it must be relinked.
void game_import::linkentity(entity &ent)
{
	impl.linkentity(&ent);
}
// call before removing an interactive edict
void game_import::unlinkentity(entity &ent)
{
	impl.unlinkentity(&ent);
}
// return entities within the specified box
dynarray<entityref> game_import::BoxEdicts(vector mins, vector maxs, box_edicts_area areatype, uint32_t allocate)
{
	dynarray<entityref> ents;
	ents.reserve(allocate);
	size_t size;

	while ((size = (size_t)impl.BoxEdicts(&mins.x, &maxs.x, (entity **)ents.data(), (int)allocate, areatype)) == (int)allocate)
	{
		allocate *= 2;
		ents.reserve(allocate);
	}

	return dynarray<entityref>(ents.data(), ents.data() + size);
}
// player movement code common with client prediction
void game_import::Pmove(pmove_t &pmove)
{
	impl.Pmove(&pmove);
}

// collision
	
// perform a line trace
[[nodiscard]] trace game_import::traceline(vector start, vector end, entityref passent, content_flags contentmask)
{
	return trace(start, vec3_origin, vec3_origin, end, passent, contentmask);
}
// perform a box trace
[[nodiscard]] trace game_import::trace(vector start, vector mins, vector maxs, vector end, entityref passent, content_flags contentmask)
{
	::trace tr = impl.trace(&start.x, &mins.x, &maxs.x, &end.x, passent, contentmask);

	if (tr.fraction == 1.0f && &tr.surface == nullptr)
	{
		gi.dprintf("Q2PRO runaway trace trapped, re-tracing...\n");
		tr = impl.trace(&start.x, &mins.x, &maxs.x, &end.x, passent, contentmask);
	}

	return tr;
}
// fetch the brush contents at the specified point
[[nodiscard]] content_flags game_import::pointcontents(vector point)
{
	return (content_flags)impl.pointcontents(&point.x);
}
// check whether the two vectors are in the same PVS
[[nodiscard]] bool game_import::inPVS(vector p1, vector p2)
{
	return impl.inPVS(&p1.x, &p2.x);
}
// check whether the two vectors are in the same PHS
[[nodiscard]] bool game_import::inPHS(vector p1, vector p2)
{
	return impl.inPHS(&p1.x, &p2.x);
}
// set the state of the specified area portal
void game_import::SetAreaPortalState(int32_t portalnum, bool open)
{
	impl.SetAreaPortalState(portalnum, open);
}
// check whether the two area indices are connected to each other
[[nodiscard]] bool game_import::AreasConnected(area_index area1, area_index area2)
{
	return impl.AreasConnected((int32_t)area1, (int32_t)area2);
}

// network

void game_import::WriteChar(int8_t c)
{
	impl.WriteChar(c);
}
void game_import::WriteByte(uint8_t c)
{
	impl.WriteByte(c);
}
void game_import::WriteShort(int16_t c)
{
	impl.WriteShort(c);
}
void game_import::WriteEntity(const entity &e)
{
	impl.WriteShort(e.s.number);
}
void game_import::WriteLong(int32_t c)
{
	impl.WriteLong(c);
}
void game_import::WriteFloat(float f)
{
	impl.WriteFloat(f);
}
void game_import::WriteString(stringlit s)
{
	impl.WriteString(s);
}
void game_import::WritePosition(vector pos)
{
	impl.WritePosition(&pos.x);
}
void game_import::WriteDir(vector pos)
{
	impl.WriteDir(&pos.x);
}
void game_import::WriteAngle(float f)
{
	impl.WriteAngle(f);
}

// send the currently-buffered message to all clients within
// the specified destination
void game_import::multicast(vector origin, multicast_destination to)
{
	impl.multicast(&origin.x, to);
}
	
// send the currently-buffered message to the specified
// client.
void game_import::unicast(const entity &ent, bool reliable)
{
	impl.unicast(const_cast<entity *>(&ent), reliable);
}

// config strings hold all the index strings, the lightstyles,
// and misc data like the sky definition and cdtrack.
// All of the current configstrings are sent to clients when
// they connect, and changes are sent to all connected clients.
void game_import::configstring(config_string num, const stringref &string)
{
	impl.configstring(num, string.ptr());
}

// parameters for ClientCommand and ServerCommand

// return arg count
[[nodiscard]] size_t game_import::argc()
{
	return impl.argc();
}
// get specific argument
[[nodiscard]] stringlit game_import::argv(size_t n)
{
	return impl.argv((int)n);
}
// get concatenation of arguments >= 1
[[nodiscard]] stringlit game_import::args()
{
	return impl.args();
}

// cvars
	
cvar &game_import::cvar_forceset(stringlit var_name, const stringref &value)
{
	return *impl.cvar_forceset(var_name, value.ptr());
}
cvar &game_import::cvar_set(stringlit var_name, const stringref &value)
{
	return *impl.cvar_set(var_name, value.ptr());
}
cvar &game_import::cvar(stringlit var_name, const stringref &value, cvar_flags flags)
{
	return *impl.cvar(var_name, value.ptr(), flags);
}

// misc

// add commands to the server console as if they were typed in
// for map changing, etc
void game_import::AddCommandString(const stringref &text)
{
	impl.AddCommandString(text.ptr());
}

void game_import::DebugGraph(float value, int color)
{
	impl.DebugGraph(value, color);
}

// drop to console, error
[[noreturn]] void game_import::error(stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string b = va(fmt, argptr);
	impl.error("%s", b.ptr());
	va_end(argptr);
}
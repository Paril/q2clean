#pragma once

// impl passed from engine.
extern "C" struct game_import_impl;

#include "config.h"
#include "lib/string.h"
#include "lib/protocol.h"
#include "lib/types/dynarray.h"
#include "lib/math/vector.h"
#include "lib/types.h"
#include "game/entity_types.h"

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
	void WriteEntity(const server_entity &e);
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
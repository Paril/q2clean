#pragma once

#include "types.h"

//
// per-level limits
//
constexpr size_t MAX_CLIENTS		= 256;	// absolute limit
constexpr size_t MAX_LIGHTSTYLES	= 256;
#ifdef KMQUAKE2_ENGINE_MOD
constexpr size_t MAX_EDICTS			= 8192;	// must change protocol to increase more
constexpr size_t MAX_MODELS			= 8192;	// these are sent over the net as bytes
constexpr size_t MAX_SOUNDS			= 8192;	// so they cannot be blindly increased
constexpr size_t MAX_IMAGES			= 2048;
#else
constexpr size_t MAX_EDICTS			= 1024;	// must change protocol to increase more
constexpr size_t MAX_MODELS			= 256;	// these are sent over the net as bytes
constexpr size_t MAX_SOUNDS			= 256;	// so they cannot be blindly increased
constexpr size_t MAX_IMAGES			= 256;
#endif
constexpr size_t MAX_ITEMS			= 256;
constexpr size_t MAX_GENERAL		= (MAX_CLIENTS * 2); // general config strings

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
enum config_string : int32_t
{
	CS_NAME,
	CS_CDTRACK,
	CS_SKY,
	CS_SKYAXIS,	// %f %f %f format
	CS_SKYROTATE,
	CS_STATUSBAR,	// display program string
	
	CS_AIRACCEL		= 29,	// air acceleration control
	CS_MAXCLIENTS,
	CS_MAPCHECKSUM,			// for catching cheater maps
	
	CS_MODELS,
	CS_SOUNDS			= CS_MODELS + MAX_MODELS,
	CS_IMAGES			= CS_SOUNDS + MAX_SOUNDS,
	CS_LIGHTS			= CS_IMAGES + MAX_IMAGES,
	CS_ITEMS			= CS_LIGHTS + MAX_LIGHTSTYLES,
	CS_PLAYERSKINS		= CS_ITEMS + MAX_ITEMS,
	CS_GENERAL			= CS_PLAYERSKINS + MAX_CLIENTS,
	MAX_CONFIGSTRINGS	= CS_GENERAL + MAX_GENERAL
};

// fetch the max size you are able to write to the specific configstring index.
// some mods actually exploit CS_STATUSBAR to take space up to CS_AIRACCEL.
constexpr size_t CS_SIZE(const config_string &cs)
{
	if (cs >= CS_STATUSBAR && cs < CS_AIRACCEL)
		return MAX_QPATH * (CS_AIRACCEL - cs);
		
	return MAX_QPATH;
}
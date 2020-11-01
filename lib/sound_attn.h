#pragma once

#include "types.h"

// sound attenuation values
enum sound_attn : uint8_t
{
	// full volume the entire level
	ATTN_NONE,
	ATTN_NORM,
	ATTN_IDLE,
	// diminish very rapidly with distance
	ATTN_STATIC
};
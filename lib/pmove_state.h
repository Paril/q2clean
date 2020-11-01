#pragma once

#include "types.h"

// pmove_state_t is the information necessary for client side movement
// prediction
enum pmtype : int32_t
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,     // different bounding box
	PM_FREEZE
};

// pmove->pm_flags
enum pmflags : uint8_t
{
	PMF_NONE,
	PMF_DUCKED			= 1 << 0,
	PMF_JUMP_HELD		= 1 << 1,
	PMF_ON_GROUND		= 1 << 2,
	// pm_time is waterjump
	PMF_TIME_WATERJUMP	= 1 << 3,
	// pm_time is time before rejump
	PMF_TIME_LAND		= 1 << 4,
	// pm_time is non-moving time
	PMF_TIME_TELEPORT	= 1 << 5,
	// temporarily disables prediction (used for grappling hook)
	PMF_NO_PREDICTION	= 1 << 6,
	// used by q2pro
	PMF_TELEPORT_BIT	= 1 << 7
};

MAKE_ENUM_BITWISE(pmflags);

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
extern "C" struct pmove_state
{
	pmtype	pm_type;

private:
#ifdef KMQUAKE2_ENGINE_MOD
	array<int32_t, 3>	origin;		// 12.3
#else
	array<int16_t, 3>	origin;		// 12.3
#endif
	array<int16_t, 3>	velocity;	// 12.3
public:
	void set_origin(const vector &v);
	vector get_origin() const;
	void set_velocity(const vector &v);
	vector get_velocity() const;

	// ducked, jump_held, etc
	pmflags	pm_flags;
	// each unit = 8 ms
	uint8_t	pm_time;
	int16_t	gravity;

private:
	// add to command angles to get view direction
	// changed by spawns, rotating objects, and teleporters
	array<int16_t, 3>	delta_angles;

public:
	void set_delta_angles(const vector &v);
	vector get_delta_angles() const;
};
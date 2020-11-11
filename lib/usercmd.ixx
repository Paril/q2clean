module;

#include "types.h"

export module usercmd;

//
// button bits
//
export enum button_bits : uint8_t
{
	BUTTON_NONE,
	BUTTON_ATTACK	= 1,
	BUTTON_USE		= 2,
	// any key whatsoever
	BUTTON_ANY		= 0xFF
};

MAKE_ENUM_BITWISE_EXPORT(button_bits);

// usercmd_t is sent to the server each client frame
export struct usercmd
{
	uint8_t				msec;
	button_bits			buttons;
private:
	array<int16_t, 3>	angles;
public:
	vector get_angles() const;
	void set_angles(vector v);

	int16_t				forwardmove, sidemove, upmove;
	// these are used, but bad
private:
	uint8_t				impulse;
	uint8_t				lightlevel;
};
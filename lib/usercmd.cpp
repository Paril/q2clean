#include "types.h"

import usercmd;

vector usercmd::get_angles() const
{
	return {
		angles[0] * short2angle,
		angles[1] * short2angle,
		angles[2] * short2angle,
	};
}

void usercmd::set_angles(vector v)
{
	angles[0] = static_cast<decltype(angles)::value_type>(v[0] * angle2short);
	angles[1] = static_cast<decltype(angles)::value_type>(v[1] * angle2short);
	angles[2] = static_cast<decltype(angles)::value_type>(v[2] * angle2short);
}

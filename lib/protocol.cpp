#include "protocol.h"

trace::trace() :
	surface(null_surface),
	contents(CONTENTS_NONE),
	ent(world)
{
}

void pmove_state::set_origin(const vector &v)
{
	for (int32_t i = 0; i < 3; i++)
		origin[i] = static_cast<decltype(origin)::value_type>(v[i] * coord2short);
}

vector pmove_state::get_origin() const
{
	return {
		origin[0] * short2coord,
		origin[1] * short2coord,
		origin[2] * short2coord
	};
}

void pmove_state::set_velocity(const vector &v)
{
	for (int32_t i = 0; i < 3; i++)
		velocity[i] = static_cast<decltype(velocity)::value_type>(v[i] * coord2short);
}

vector pmove_state::get_velocity() const
{
	return {
		velocity[0] * short2coord,
		velocity[1] * short2coord,
		velocity[2] * short2coord
	};
}

void pmove_state::set_delta_angles(const vector &v)
{
	for (int32_t i = 0; i < 3; i++)
		delta_angles[i] = static_cast<decltype(delta_angles)::value_type>(v[i] * angle2short);
}

vector pmove_state::get_delta_angles() const
{
	return {
		delta_angles[0] * short2angle,
		delta_angles[1] * short2angle,
		delta_angles[2] * short2angle,
	};
}

/*static*/ qboolean cvarref::empty_modified = false;

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
	for (int32_t i = 0; i < 3; i++)
		angles[i] = static_cast<decltype(angles)::value_type>(v[i] * angle2short);
}
#include "pmove_state.h"

void pmove_state::set_origin(const vector &v)
{
	origin[0] = static_cast<decltype(origin)::value_type>(v[0] * coord2short);
	origin[1] = static_cast<decltype(origin)::value_type>(v[1] * coord2short);
	origin[2] = static_cast<decltype(origin)::value_type>(v[2] * coord2short);
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
	velocity[0] = static_cast<decltype(velocity)::value_type>(v[0] * coord2short);
	velocity[1] = static_cast<decltype(velocity)::value_type>(v[1] * coord2short);
	velocity[2] = static_cast<decltype(velocity)::value_type>(v[2] * coord2short);
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
	delta_angles[0] = static_cast<decltype(delta_angles)::value_type>(v[0] * angle2short);
	delta_angles[1] = static_cast<decltype(delta_angles)::value_type>(v[1] * angle2short);
	delta_angles[2] = static_cast<decltype(delta_angles)::value_type>(v[2] * angle2short);
}

vector pmove_state::get_delta_angles() const
{
	return {
		delta_angles[0] * short2angle,
		delta_angles[1] * short2angle,
		delta_angles[2] * short2angle,
	};
}
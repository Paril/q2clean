#include "config.h"
#include "gi.h"
#include "lib/string/format.h"

game_import gi;

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

// sounds

// fetch a sound index from the specified sound file
sound_index game_import::soundindex(const stringref &name)
{
	return (sound_index) impl.soundindex(name.ptr());
}

// sound; play soundindex on specified entity on channel at specified volume/attn with a time offset of timeofs
void game_import::sound(const entity &ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float time_offset)
{
	if ((channel & (CHAN_NO_PHS_ADD | CHAN_RELIABLE)) == (CHAN_NO_PHS_ADD | CHAN_RELIABLE))
		gi.dprint("invalid sound parameters passed; no_phs | reliable is not supported in most engines\n");

	impl.sound(const_cast<entity *>(&ent), channel, (int32_t) soundindex, volume, attenuation, time_offset);
}

// sound; play soundindex at specified origin (from specified entity) on channel at specified volume/attn with a time offset of timeofs
void game_import::positioned_sound(vector origin, entityref ent, sound_channel channel, sound_index soundindex, float volume, sound_attn attenuation, float time_offset)
{
	if ((channel & (CHAN_NO_PHS_ADD | CHAN_RELIABLE)) == (CHAN_NO_PHS_ADD | CHAN_RELIABLE))
		gi.dprint("invalid sound parameters passed; no_phs | reliable is not supported in most engines\n");

	impl.positioned_sound(&origin.x, ent, channel, (int32_t) soundindex, volume, attenuation, time_offset);
}

// models

// fetch a model index from the specified sound file
model_index game_import::modelindex(const stringref &name)
{
	return (model_index) impl.modelindex(name.ptr());
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
	return (image_index) impl.imageindex(name.ptr());
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
dynarray<entityref> game_import::BoxEdicts(bbox bounds, box_edicts_area areatype, uint32_t allocate)
{
	dynarray<entityref> ents;
	ents.reserve(allocate);
	size_t size;

	while ((size = (size_t) impl.BoxEdicts(&bounds.mins.x, &bounds.maxs.x, (entity **) ents.data(), (int32_t) allocate, areatype)) == allocate)
	{
		allocate *= 2;
		ents.reserve(allocate);
	}

	return dynarray<entityref>(ents.data(), ents.data() + size);
}
// player movement code common with client prediction
void game_import::Pmove(pmove &pmove)
{
	impl.Pmove(&pmove);
}

// collision

// perform a line trace
[[nodiscard]] trace game_import::traceline(vector start, vector end, entityref passent, content_flags contentmask)
{
	return trace(start, bbox_point, end, passent, contentmask);
}
// perform a box trace
[[nodiscard]] trace game_import::trace(vector start, bbox bounds, vector end, entityref passent, content_flags contentmask)
{
	::trace tr = impl.trace(&start.x, &bounds.mins.x, &bounds.maxs.x, &end.x, passent, contentmask);

	if (tr.fraction == 1.0f && *(void **)(&tr.contents - 1) == nullptr)
	{
		gi.dprint("Q2PRO runaway trace trapped, re-tracing...\n");
		tr = impl.trace(&start.x, &bounds.mins.x, &bounds.maxs.x, &end.x, passent, contentmask);
	}

	return tr;
}
// fetch the brush contents at the specified point
[[nodiscard]] content_flags game_import::pointcontents(vector point)
{
	return (content_flags) impl.pointcontents(&point.x);
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
	return impl.AreasConnected((int32_t) area1, (int32_t) area2);
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
void game_import::WriteEntity(const server_entity &e)
{
	impl.WriteShort(e.number);
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
void game_import::WriteDir(vector dir)
{
	impl.WriteDir(&dir.x);
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
	return impl.argv((int) n);
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
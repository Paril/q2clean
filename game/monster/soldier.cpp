#include "config.h"

#ifdef SINGLE_PLAYER
#include "../entity.h"
#include "../game.h"
#include "../move.h"
#include "../ai.h"
#include "../monster.h"
#include "../misc.h"
#include "../spawn.h"
#include "lib/gi.h"
#include "flash.h"
#include "game/util.h"
#include "soldier_model.h"
#include "soldier.h"

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif

static sound_index	sound_idle;
static sound_index	sound_sight1;
static sound_index	sound_sight2;
static sound_index	sound_pain_light;
static sound_index	sound_pain;
static sound_index	sound_pain_ss;
static sound_index	sound_death_light;
static sound_index	sound_death;
static sound_index	sound_death_ss;
static sound_index	sound_cock;

#ifdef THE_RECKONING
static model_index	soldierh_modelindex;
#include "game/xatrix/monster.h"
#endif

static void soldier_idle(entity &self)
{
	if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_idle, ATTN_IDLE);
}

static void soldier_cock(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_cock, self.frame == FRAME_stand322 ? ATTN_IDLE : ATTN_NORM);
}

// STAND
static void soldier_stand(entity &self);

constexpr mframe_t soldier_frames_stand1 [] = {
	{ ai_stand, 0, soldier_idle },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
constexpr mmove_t soldier_move_stand1 = { FRAME_stand101, FRAME_stand130, soldier_frames_stand1, soldier_stand };

REGISTER_STATIC_SAVABLE(soldier_move_stand1);

constexpr mframe_t soldier_frames_stand3 [] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand, 0, soldier_cock },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
constexpr mmove_t soldier_move_stand3 = { FRAME_stand301, FRAME_stand339, soldier_frames_stand3, soldier_stand };

REGISTER_STATIC_SAVABLE(soldier_move_stand3);

static void soldier_stand(entity &self)
{
	if ((self.monsterinfo.currentmove == &soldier_move_stand3) || (random() < 0.8f))
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_stand1);
	else
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_stand3);
}

REGISTER_STATIC_SAVABLE(soldier_stand);

//
// WALK
//

static void soldier_walk1_random(entity &self)
{
	if (random() > 0.1f)
		self.monsterinfo.nextframe = FRAME_walk101;
}

constexpr mframe_t soldier_frames_walk1 [] = {
	{ ai_walk, 3 },
	{ ai_walk, 6 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 1 },
	{ ai_walk, 6 },
	{ ai_walk, 5 },
	{ ai_walk, 3 },
	{ ai_walk, -1, soldier_walk1_random },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk }
};
constexpr mmove_t soldier_move_walk1 = { FRAME_walk101, FRAME_walk133, soldier_frames_walk1 };

REGISTER_STATIC_SAVABLE(soldier_move_walk1);

constexpr mframe_t soldier_frames_walk2 [] = {
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 9 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },
	{ ai_walk, 1 },
	{ ai_walk, 3 },
	{ ai_walk, 7 },
	{ ai_walk, 6 },
	{ ai_walk, 7 }
};
constexpr mmove_t soldier_move_walk2 = { FRAME_walk209, FRAME_walk218, soldier_frames_walk2 };

REGISTER_STATIC_SAVABLE(soldier_move_walk2);

static void soldier_walk(entity &self)
{
	if (random() < 0.5f)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_walk1);
	else
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_walk2);
}

REGISTER_STATIC_SAVABLE(soldier_walk);

//
// RUN
//
static void soldier_run(entity &self);

constexpr mframe_t soldier_frames_start_run [] = {
	{ ai_run, 7 },
	{ ai_run, 5 }
};
constexpr mmove_t soldier_move_start_run = { FRAME_run01, FRAME_run02, soldier_frames_start_run, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_start_run);

#ifdef ROGUE_AI
static void soldier_start_charge (entity &self)
{
	self.monsterinfo.aiflags |= AI_CHARGING;
}

static void soldier_stop_charge (entity &self)
{
	self.monsterinfo.aiflags &= ~AI_CHARGING;
}
#endif

constexpr mframe_t soldier_frames_run [] = {
	{ ai_run, 10 },
	{ ai_run, 11
#ifdef ROGUE_AI
		, monster_done_dodge
#endif
	},
	{ ai_run, 11 },
	{ ai_run, 16 },
	{ ai_run, 10 },
	{ ai_run, 15
#ifdef ROGUE_AI
		, monster_done_dodge
#endif
	}
};
constexpr mmove_t soldier_move_run = { FRAME_run03, FRAME_run08, soldier_frames_run };

REGISTER_STATIC_SAVABLE(soldier_move_run);

static void soldier_run(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);
#endif

	if (self.monsterinfo.aiflags & AI_STAND_GROUND) {
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_stand1);
		return;
	}

	if (self.monsterinfo.currentmove == &soldier_move_walk1 ||
		self.monsterinfo.currentmove == &soldier_move_walk2 ||
		self.monsterinfo.currentmove == &soldier_move_start_run) {
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_run);
	} else {
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_start_run);
	}
}

REGISTER_STATIC_SAVABLE(soldier_run);

//
// PAIN
//

constexpr mframe_t soldier_frames_pain1 [] = {
	{ ai_move, -3 },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move }
};
constexpr mmove_t soldier_move_pain1 = { FRAME_pain101, FRAME_pain105, soldier_frames_pain1, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_pain1);

constexpr mframe_t soldier_frames_pain2 [] = {
	{ ai_move, -13 },
	{ ai_move, -1 },
	{ ai_move, 2 },
	{ ai_move, 4 },
	{ ai_move, 2 },
	{ ai_move, 3 },
	{ ai_move, 2 }
};
constexpr mmove_t soldier_move_pain2 = { FRAME_pain201, FRAME_pain207, soldier_frames_pain2, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_pain2);

constexpr mframe_t soldier_frames_pain3 [] = {
	{ ai_move, -8 },
	{ ai_move, 10 },
	{ ai_move, -4 },
	{ ai_move, -1 },
	{ ai_move, -3 },
	{ ai_move },
	{ ai_move, 3 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move, 2 },
	{ ai_move, 4 },
	{ ai_move, 3 },
	{ ai_move, 2 }
};
constexpr mmove_t soldier_move_pain3 = { FRAME_pain301, FRAME_pain318, soldier_frames_pain3, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_pain3);

constexpr mframe_t soldier_frames_pain4 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -10 },
	{ ai_move, -6 },
	{ ai_move, 8 },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move, 5 },
	{ ai_move, 2 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, 3 },
	{ ai_move, 2 },
	{ ai_move }
};
constexpr mmove_t soldier_move_pain4 = { FRAME_pain401, FRAME_pain417, soldier_frames_pain4, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_pain4);

#ifdef THE_RECKONING
static void soldierh_end_hyper_sound(entity &self)
{
	if (self.modelindex == soldierh_modelindex && (self.skinnum == 2 || self.skinnum == 3))
	{
		self.sound = SOUND_NONE;
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"));
	}
}
#endif

static void soldier_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);
	soldier_stop_charge(self);

	// if we're blind firing, this needs to be turned off here
	self.monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
#endif

	if (level.framenum < self.pain_debounce_framenum)
	{
		if ((self.velocity.z > 100) && ((self.monsterinfo.currentmove == &soldier_move_pain1) || (self.monsterinfo.currentmove == &soldier_move_pain2) || (self.monsterinfo.currentmove == &soldier_move_pain3)))
		{
#ifdef ROGUE_AI
			if (self.monsterinfo.aiflags & AI_DUCKED)
				monster_duck_up(self);
#endif
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_pain4);
		}
		return;
	}

	self.pain_debounce_framenum = level.framenum + 3s;

	const int32_t n = self.skinnum | 1;
	if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain_light);
	else if (n == 3)
		gi.sound(self, CHAN_VOICE, sound_pain);
	else
		gi.sound(self, CHAN_VOICE, sound_pain_ss);

	if (self.velocity.z > 100)
	{
#ifdef ROGUE_AI
		if (self.monsterinfo.aiflags & AI_DUCKED)
			monster_duck_up(self);
#endif

		self.monsterinfo.currentmove = &SAVABLE(soldier_move_pain4);
		return;
	}

	if (skill == 3)
		return;     // no pain anims in nightmare

	float r = random();

	if (r < 0.33f)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_pain1);
	else if (r < 0.66f)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_pain2);
	else
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_pain3);

#ifdef THE_RECKONING
	soldierh_end_hyper_sound(self);
#endif

#ifdef ROGUE_AI
	if (self.monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
#endif
}

REGISTER_STATIC_SAVABLE(soldier_reacttodamage);

//
// ATTACK
//

#ifdef THE_RECKONING
static void soldierh_laserbeam(entity &self, monster_muzzleflash flash_index)
{
	vector	forward, right, up;
	vector	tempang, start;
	vector	dir, angles, end;
	vector	tempvec;

	// RAFAEL
	// this sound can't be called this frequent
	if ((level.framenum % 800ms) == gtime::zero())
		gi.sound(self, CHAN_WEAPON, gi.soundindex("misc/lasfly.wav"), ATTN_STATIC);

	start = self.origin;
	end = self.enemy->origin;
	dir = end - start;
	angles = vectoangles (dir);
	tempvec = monster_flash_offset[flash_index];
	
	entity &ent = G_Spawn ();
	ent.origin = self.origin;
	tempang = angles;
	AngleVectors (tempang, &forward, &right, &up);
	ent.angles = tempang;
	start = ent.origin;

	if (flash_index == 85)
		start += right * (tempvec[0] - 14);
	else 
		start += right * (tempvec[0] + 2);

	start += up * (tempvec[2] + 8);
	start += forward * tempvec[1];
			
	ent.origin = start;
	ent.enemy = self.enemy;
	ent.owner = self;
	
	ent.dmg = 1;

	monster_dabeam (ent);
}
#endif

constexpr size_t num_flashes = 8;
static constexpr monster_muzzleflash blaster_flash[num_flashes] = {MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2, MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_BLASTER_8};
static constexpr monster_muzzleflash shotgun_flash[num_flashes] = {MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_SHOTGUN_8};
static constexpr monster_muzzleflash machinegun_flash[num_flashes] = {MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2, MZ2_SOLDIER_MACHINEGUN_3, MZ2_SOLDIER_MACHINEGUN_4, MZ2_SOLDIER_MACHINEGUN_5, MZ2_SOLDIER_MACHINEGUN_6, MZ2_SOLDIER_MACHINEGUN_7, MZ2_SOLDIER_MACHINEGUN_8};

static void soldier_fire(entity &self, int32_t flash_number)
{
	vector	start;
	vector	forward, right, up;
	vector	aim;
	vector	dir;
	vector	end;
	float	r, u;
	monster_muzzleflash	flash_index;
#ifdef ROGUE_AI
	int32_t	in_flash_number = flash_number;
	vector	aim_norm;
	float	angle;
	trace	tr;
	vector aim_good;

	if (!self.enemy.has_value() || !self.enemy->inuse)
	{
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		return;
	}

	if (in_flash_number < 0)
		flash_number = -in_flash_number;
	else
		flash_number = in_flash_number;
#endif

	if (self.skinnum < 2)
		flash_index = blaster_flash[flash_number];
	else if (self.skinnum < 4)
#ifdef THE_RECKONING
		flash_index = (self.modelindex == soldierh_modelindex) ? blaster_flash[flash_number] : shotgun_flash[flash_number];
#else
		flash_index = shotgun_flash[flash_number];
#endif
	else
		flash_index = machinegun_flash[flash_number];

	AngleVectors(self.angles, &forward, &right, nullptr);
	start = G_ProjectSource(self.origin, monster_flash_offset[flash_index], forward, right);

	if (flash_number == 5 || flash_number == 6)
	{
		aim = forward;
	}
	else
	{
		end = self.enemy->origin;
		end.z += self.enemy->viewheight;
		aim = end - start;
		
#ifdef ROGUE_AI
		aim_good = end;

		if (in_flash_number < 0)
		{
			aim_norm = aim;
			VectorNormalize (aim_norm);
			angle = aim_norm * forward;
			if (angle < 0.9)  // ~25 degree angle
				return;
		}
#endif
		
		dir = vectoangles(aim);
		AngleVectors(dir, &forward, &right, &up);

#ifdef THE_RECKONING
		if (self.modelindex == soldierh_modelindex)
		{
			r = random(-100.f, 100.f);
			u = random(-50.f, 50.f);
		}
		else
		{
#endif
#ifdef ROGUE_AI
		if (skill < 2)
		{
#endif
			r = random(-1000.f, 1000.f);
			u = random(-500.f, 500.f);
#ifdef ROGUE_AI
		}
		else
		{
			r = random(-500.f, 500.f);
			u = random(-250.f, 250.f);
		}
#endif
#ifdef THE_RECKONING
		}
#endif
		end = start + (8192 * forward);
		end += (r * right);
		end += (u * up);

		aim = end - start;
		VectorNormalize(aim);
		
#ifdef ROGUE_AI
		if (!(flash_number == 5 || flash_number == 6))
		{
			tr = gi.traceline(start, aim_good, self, MASK_SHOT);
			if ((tr.ent != self.enemy) && !tr.ent.is_world())
				return;
		}
#endif
	}

	if (self.skinnum <= 1) {
#ifdef THE_RECKONING
		if (self.modelindex == soldierh_modelindex)
			monster_fire_ionripper (self, start, aim, 5, 600, flash_index, EF_IONRIPPER);
		else
#endif
			monster_fire_blaster(self, start, aim, 5, 600, flash_index, EF_BLASTER);
	} else if (self.skinnum <= 3) {
#ifdef THE_RECKONING
		if (self.modelindex == soldierh_modelindex)
		{
			self.sound = gi.soundindex("weapons/hyprbl1a.wav");
			monster_fire_blueblaster(self, start, aim, 1, 600, flash_index, EF_BLUEHYPERBLASTER);
		}
		else
#endif
			monster_fire_shotgun(self, start, aim, 2, 1, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SHOTGUN_COUNT, flash_index);
	} else {
		if (!(self.monsterinfo.aiflags & AI_HOLD_FRAME))
#ifdef ROGUE_AI
			self.wait =
#else
			self.monsterinfo.pause_framenum = 
#endif
				level.framenum + 300ms + random(700ms);

#ifdef THE_RECKONING
		if (self.modelindex == soldierh_modelindex)
			soldierh_laserbeam (self, flash_index);
		else
#endif
			monster_fire_bullet(self, start, aim, 2, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_index);

		if (level.framenum >= 
#ifdef ROGUE_AI
			self.wait
#else
			self.monsterinfo.pause_framenum
#endif
			)
			self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self.monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}

// ATTACK1 (blaster/shotgun)

#ifdef THE_RECKONING
static void soldierh_hyper_refire1(entity &self)
{
	if (self.modelindex != soldierh_modelindex || self.skinnum < 2)
		return;
	else if (self.skinnum < 4)
	{
		if (random() < 0.7)
			self.frame = FRAME_attak103;
		else
			soldierh_end_hyper_sound(self);
	}
}

static void soldierh_ripper1(entity &self)
{
	if (self.modelindex != soldierh_modelindex)
		return;

	if (self.skinnum < 2)
		soldier_fire (self, 0);
	else if (self.skinnum < 4)
		soldier_fire (self, 0);
}
#endif

static void soldier_fire1(entity &self)
{
	soldier_fire(self, 0);
}

static void soldier_attack1_refire1(entity &self)
{
#ifdef ROGUE_AI
	if (self.monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self.monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return;
	}
#endif

	if (!self.enemy.has_value())
		return;

	if (self.skinnum > 1)
		return;

	if (self.enemy->health <= 0)
		return;

	if (((skill == 3) && (random() < 0.5f)) || (range(self, self.enemy) == RANGE_MELEE))
		self.monsterinfo.nextframe = FRAME_attak102;
	else
		self.monsterinfo.nextframe = FRAME_attak110;
}

static void soldier_attack1_refire2(entity &self)
{
	if (!self.enemy.has_value())
		return;

	if (self.skinnum < 2)
		return;

	if (self.enemy->health <= 0)
		return;

	if (((skill == 3) && (random() < 0.5f)) || (range(self, self.enemy) == RANGE_MELEE))
		self.monsterinfo.nextframe = FRAME_attak102;
}

constexpr mframe_t soldier_frames_attack1 [] = {
	{ ai_charge },
#ifdef THE_RECKONING
	{ ai_charge },
	{ ai_charge, 0,  soldier_fire1 },
	{ ai_charge, 0,  soldierh_ripper1 },
	{ ai_charge, 0,  soldierh_ripper1 },
	{ ai_charge, 0,  soldier_attack1_refire1 },
	{ ai_charge, 0,  soldierh_hyper_refire1 },
#else
	{ ai_charge },
	{ ai_charge, 0,  soldier_fire1 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0,  soldier_attack1_refire1 },
	{ ai_charge },
#endif
	{ ai_charge, 0,  soldier_cock },
	{ ai_charge, 0,  soldier_attack1_refire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t soldier_move_attack1 = { FRAME_attak101, FRAME_attak112, soldier_frames_attack1, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_attack1);

// ATTACK2 (blaster/shotgun)

#ifdef THE_RECKONING
static void soldierh_hyper_refire2(entity &self)
{
	if (!self.enemy.has_value())
		return;

	if (self.modelindex != soldierh_modelindex || self.skinnum < 2)
		return;
	else if (self.skinnum < 4)
	{
		if (random() < 0.7)
			self.frame = FRAME_attak205;
		else
			soldierh_end_hyper_sound(self);
	}
}

static void soldierh_ripper2(entity &self)
{
	if (self.modelindex != soldierh_modelindex)
		return;

	if (self.skinnum < 2)
		soldier_fire (self, 1);
	else if (self.skinnum < 4)
		soldier_fire (self, 1);
}
#endif

static void soldier_fire2(entity &self)
{
	soldier_fire(self, 1);
}

static void soldier_attack2_refire1(entity &self)
{
	if (!self.enemy.has_value())
		return;

	if (self.skinnum > 1)
		return;

	if (self.enemy->health <= 0)
		return;

	if (((skill == 3) && (random() < 0.5f)) || (range(self, self.enemy) == RANGE_MELEE))
		self.monsterinfo.nextframe = FRAME_attak204;
	else
		self.monsterinfo.nextframe = FRAME_attak216;
}

static void soldier_attack2_refire2(entity &self)
{
	if (!self.enemy.has_value())
		return;

	if (self.skinnum < 2)
		return;

	if (self.enemy->health <= 0)
		return;

	if (((skill == 3) && (random() < 0.5f)) || (range(self, self.enemy) == RANGE_MELEE))
		self.monsterinfo.nextframe = FRAME_attak204;
}

constexpr mframe_t soldier_frames_attack2 [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
#ifdef THE_RECKONING
	{ ai_charge },
	{ ai_charge, 0, soldier_fire2 },
	{ ai_charge, 0, soldierh_ripper2 },
	{ ai_charge, 0, soldierh_ripper2 },
	{ ai_charge, 0, soldier_attack2_refire1 },
	{ ai_charge, 0, soldierh_hyper_refire2 },
#else
	{ ai_charge },
	{ ai_charge, 0, soldier_fire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_refire1 },
	{ ai_charge },
#endif
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_cock },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_refire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t soldier_move_attack2 = { FRAME_attak201, FRAME_attak218, soldier_frames_attack2, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_attack2);

// ATTACK3 (duck and shoot)

#ifdef ROGUE_AI
#define soldier_duck_down monster_duck_down
#define soldier_duck_up monster_duck_up
#define soldier_duck_hold monster_duck_hold
#else
static void soldier_duck_down(entity &self)
{
	if (self.monsterinfo.aiflags & AI_DUCKED)
		return;
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.maxs.z -= 32;
	self.takedamage = true;
	self.monsterinfo.pause_framenum = level.framenum + 1 * BASE_FRAMERATE;
	gi.linkentity(self);
}

static void soldier_duck_up(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_DUCKED;
	self.maxs.z += 32;
	self.takedamage = true;
	gi.linkentity(self);
}
#endif

static void soldier_fire3(entity &self)
{
#ifdef GROUND_ZERO
	if (!(self.monsterinfo.aiflags & AI_DUCKED))
#endif
		soldier_duck_down(self);
	soldier_fire(self, 2);
}

static void soldier_attack3_refire(entity &self)
{
	if ((level.framenum + 400ms) <
#ifdef ROGUE_AI
		self.monsterinfo.duck_wait_framenum
#else
		self.monsterinfo.pause_framenum
#endif
		)
		self.monsterinfo.nextframe = FRAME_attak303;
}

constexpr mframe_t soldier_frames_attack3 [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_fire3 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack3_refire },
	{ ai_charge, 0, soldier_duck_up },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t soldier_move_attack3 = { FRAME_attak301, FRAME_attak309, soldier_frames_attack3, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_attack3);

// ATTACK4 (machinegun)

static void soldier_fire4(entity &self)
{
	soldier_fire(self, 3);
}

constexpr mframe_t soldier_frames_attack4 [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_fire4 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t soldier_move_attack4 = { FRAME_attak401, FRAME_attak406, soldier_frames_attack4, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_attack4);

// ATTACK6 (run & shoot)

static void soldier_fire8(entity &self)
{
#ifdef ROGUE_AI
	soldier_fire(self, -7);
#else
	soldier_fire(self, 7);
#endif
}

static void soldier_attack6_refire(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);
	soldier_stop_charge (self);
#endif

	if (!self.enemy.has_value())
		return;

	if (self.enemy->health <= 0)
		return;

	if (range(self, self.enemy) <
#ifdef ROGUE_AI
		RANGE_NEAR
#else
		RANGE_MID
#endif
		)
		return;

	if (skill == 3
#ifdef ROGUE_AI
		 || (random() < (0.25f * skill))
#endif
		)
		self.monsterinfo.nextframe = FRAME_runs03;
}

constexpr mframe_t soldier_frames_attack6 [] =
{
	{ ai_charge, 10
#ifdef ROGUE_AI
		, soldier_start_charge
#endif
	},
	{ ai_charge,  4 },
	{ ai_charge, 12 },
	{ ai_charge, 11, soldier_fire8 },
	{ ai_charge, 13 },
	{ ai_charge, 18
#ifdef ROGUE_AI
		, monster_done_dodge
#endif
	},
	{ ai_charge, 15 },
	{ ai_charge, 14 },
	{ ai_charge, 11 },
	{ ai_charge,  8 },
	{ ai_charge, 11 },
	{ ai_charge, 12 },
	{ ai_charge, 12 },
	{ ai_charge, 17, soldier_attack6_refire }
};
constexpr mmove_t soldier_move_attack6 = { FRAME_runs01, FRAME_runs14, soldier_frames_attack6, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_attack6);

static void soldier_attack(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);

	// PMM - blindfire!
	if (self.monsterinfo.attack_state == AS_BLIND)
	{
		float chance = 0.1f;
		// setup shot probabilities
		if (self.monsterinfo.blind_fire_framedelay < 1s)
			chance = 1.0f;
		else if (self.monsterinfo.blind_fire_framedelay < 7.5s)
			chance = 0.4f;

		float r = random();

		// minimum of 2 seconds, plus 0-3, after the shots are done
		self.monsterinfo.blind_fire_framedelay += duration_cast<gtime>(random(4.1s, 7.1s));

		// don't shoot at the origin
		if (!self.monsterinfo.blind_fire_target)
			return;

		// don't shoot if the dice say not to
		if (r > chance)
			return;

		// turn on manual steering to signal both manual steering and blindfire
		self.monsterinfo.aiflags |= AI_MANUAL_STEERING;
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack1);
		self.monsterinfo.attack_finished = level.framenum + duration_cast<gtime>(random(1.5s, 2.5s));
		return;
	}

	float r = random();

	if ((!(self.monsterinfo.aiflags & (AI_BLOCKED|AI_STAND_GROUND))) &&
		(range(self, self.enemy) >= RANGE_NEAR) && 
		(r < (skill * 0.25) && 
		(self.skinnum <= 3)) &&
		(visible(self, self.enemy)))
	{
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack6);
		return;
	}
#endif

	if (self.skinnum < 4)
	{
		if (random() < 0.5f)
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack1);
		else
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack2);
	}
	else
	{
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack4);
	}
}

REGISTER_STATIC_SAVABLE(soldier_attack);

//
// SIGHT
//

static void soldier_sight(entity &self, entity &)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_sight1);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2);

	if (skill && self.enemy.has_value() && (range(self, self.enemy) >= RANGE_MID) && visible(self, self.enemy)) {
		if (random() > 0.5f)
#if defined(THE_RECKONING) || defined(GROUND_ZERO)
		{
			if (self.skinnum < 4)	
				self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack6);
			else
				self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack4);
		}
#else
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack6);
#endif
	}
}

REGISTER_STATIC_SAVABLE(soldier_sight);

//
// DUCK
//

#ifndef ROGUE_AI
static void soldier_duck_hold(entity &self)
{
	if (level.framenum >= self.monsterinfo.pause_framenum)
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self.monsterinfo.aiflags |= AI_HOLD_FRAME;
}
#endif

constexpr mframe_t soldier_frames_duck [] = {
	{ ai_move, 5, soldier_duck_down },
	{ ai_move, -1, soldier_duck_hold },
	{ ai_move, 1 },
	{ ai_move, 0,  soldier_duck_up },
	{ ai_move, 5 }
};
constexpr mmove_t soldier_move_duck = { FRAME_duck01, FRAME_duck05, soldier_frames_duck, soldier_run };

REGISTER_STATIC_SAVABLE(soldier_move_duck);

#ifndef ROGUE_AI
static void soldier_dodge(entity &self, entity &attacker, float eta)
{
	float	r;

	r = random();
	if (r > 0.25f)
		return;

	if (!self.enemy.has_value())
		self.enemy = attacker;

#ifdef THE_RECKONING
	soldierh_end_hyper_sound(self);
#endif

	if (!skill) {
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_duck);
		return;
	}

	self.monsterinfo.pause_framenum = level.framenum + (int)((eta + 0.3f) * BASE_FRAMERATE);
	r = random();

	if (skill == 1) {
		if (r > 0.33f)
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_duck);
		else
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack3);
		return;
	}

	if (skill >= 2) {
		if (r > 0.66f)
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_duck);
		else
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack3);
		return;
	}

	self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack3);
}

REGISTER_STATIC_SAVABLE(soldier_dodge);
#endif

//
// DEATH
//

static void soldier_fire6(entity &self)
{
#ifdef THE_RECKONING
	// no fire laser
	if (self.modelindex != soldierh_modelindex || self.skinnum < 4)
#endif
		soldier_fire(self, 5);
}

static void soldier_fire7(entity &self)
{
#ifdef THE_RECKONING
	// no fire laser
	if (self.modelindex != soldierh_modelindex || self.skinnum < 4)
#endif
		soldier_fire(self, 6);
}

static void soldier_dead(entity &self)
{
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, -8 }
	};
	self.movetype = MOVETYPE_TOSS;
	self.svflags |= SVF_DEADMONSTER;
	self.nextthink = gtime::zero();
	gi.linkentity(self);
}

constexpr mframe_t soldier_frames_death1 [] = {
	{ ai_move },
	{ ai_move, -10 },
	{ ai_move, -10 },
	{ ai_move, -10 },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move, 0,   soldier_fire6 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0,   soldier_fire7 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_death1 = { FRAME_death101, FRAME_death136, soldier_frames_death1, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death1);

constexpr mframe_t soldier_frames_death2 [] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_death2 = { FRAME_death201, FRAME_death235, soldier_frames_death2, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death2);

constexpr mframe_t soldier_frames_death3 [] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};
constexpr mmove_t soldier_move_death3 = { FRAME_death301, FRAME_death345, soldier_frames_death3, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death3);

constexpr mframe_t soldier_frames_death4 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_death4 = { FRAME_death401, FRAME_death453, soldier_frames_death4, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death4);

constexpr mframe_t soldier_frames_death5 [] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_death5 = { FRAME_death501, FRAME_death524, soldier_frames_death5, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death5);

constexpr mframe_t soldier_frames_death6 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_death6 = { FRAME_death601, FRAME_death610, soldier_frames_death6, soldier_dead };

REGISTER_STATIC_SAVABLE(soldier_move_death6);

static void soldier_die(entity &self, entity &, entity &, int32_t damage, vector point)
{
	int	n;

#ifdef THE_RECKONING
	soldierh_end_hyper_sound(self);
#endif

// check for gib
	if (self.health <= self.gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"));
		for (n = 0; n < 3; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self.deadflag = DEAD_DEAD;
		return;
	}

	if (self.deadflag == DEAD_DEAD)
		return;

// regular death
	self.deadflag = DEAD_DEAD;
	self.takedamage = true;
	self.skinnum |= 1;

	if (self.skinnum == 1)
		gi.sound(self, CHAN_VOICE, sound_death_light);
	else if (self.skinnum == 3)
		gi.sound(self, CHAN_VOICE, sound_death);
	else // (self.skinnum == 5)
		gi.sound(self, CHAN_VOICE, sound_death_ss);

	if (fabs((self.origin[2] + self.viewheight) - point[2]) <= 4) {
		// head shot
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death3);
		return;
	}

	n = Q_rand() % 5;
	if (n == 0)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death1);
	else if (n == 1)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death2);
	else if (n == 2)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death4);
	else if (n == 3)
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death5);
	else
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_death6);
}

REGISTER_STATIC_SAVABLE(soldier_die);

#ifdef ROGUE_AI
//
// NEW DODGE CODE
//

static void soldier_sidestep (entity &self)
{
	if (self.skinnum <= 3)
	{
		if (self.monsterinfo.currentmove != &soldier_move_attack6)
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack6);
	}
	else
	{
		if (self.monsterinfo.currentmove != &soldier_move_start_run)
			self.monsterinfo.currentmove = &SAVABLE(soldier_move_start_run);
	}
}

REGISTER_STATIC_SAVABLE(soldier_sidestep);

static void soldier_duck (entity &self, gtimef eta)
{
	float r;

	// has to be done immediately otherwise he can get stuck
	monster_duck_down(self);

	if (!skill)
	{
		// PMM - stupid dodge
		self.monsterinfo.nextframe = FRAME_duck01;
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_duck);
		self.monsterinfo.duck_wait_framenum = duration_cast<gtime>(level.framenum + eta + 1s);

#ifdef THE_RECKONING
		soldierh_end_hyper_sound(self);
#endif
		return;
	}

	r = random();

	if (r > (skill * 0.3))
	{
		self.monsterinfo.nextframe = FRAME_duck01;
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_duck);
		self.monsterinfo.duck_wait_framenum = duration_cast<gtime>(level.framenum + eta + (100ms * (3 - skill)));
	}
	else
	{
		self.monsterinfo.nextframe = FRAME_attak301;
		self.monsterinfo.currentmove = &SAVABLE(soldier_move_attack3);
		self.monsterinfo.duck_wait_framenum = duration_cast<gtime>(level.framenum + eta + 1s);
	}

#ifdef THE_RECKONING
	soldierh_end_hyper_sound(self);
#endif
	return;
}

REGISTER_STATIC_SAVABLE(soldier_duck);

static void soldier_blind (entity &self);

constexpr mframe_t soldier_frames_blind [] =
{
	{ ai_move, 0, soldier_idle },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t soldier_move_blind = { FRAME_stand101, FRAME_stand130, soldier_frames_blind, soldier_blind };

REGISTER_STATIC_SAVABLE(soldier_move_blind);

static void soldier_blind (entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(soldier_move_blind);
}

REGISTER_STATIC_SAVABLE(soldier_blind);

static bool soldier_blocked (entity &self, float dist)
{
	// don't do anything if you're dodging
	if ((self.monsterinfo.aiflags & AI_DODGING) || (self.monsterinfo.aiflags & AI_DUCKED))
		return false;

	if(blocked_checkshot (self, 0.25f + (0.05f * skill)))
		return true;

	if(blocked_checkplat (self, dist))
		return true;

	return false;
}

REGISTER_STATIC_SAVABLE(soldier_blocked);
#endif

//
// SPAWN
//

inline void SP_monster_soldier_x(entity &self, stringlit model)
{
	self.modelindex = gi.modelindex(model);
	self.monsterinfo.scale = MODEL_SCALE;
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, 32 }
	};
	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;

	sound_idle =    gi.soundindex("soldier/solidle1.wav");
	sound_sight1 =  gi.soundindex("soldier/solsght1.wav");
	sound_sight2 =  gi.soundindex("soldier/solsrch1.wav");
	sound_cock =    gi.soundindex("infantry/infatck3.wav");

	self.mass = 100;

	self.pain = SAVABLE(monster_pain);
	self.die = SAVABLE(soldier_die);

	self.monsterinfo.stand = SAVABLE(soldier_stand);
	self.monsterinfo.walk = SAVABLE(soldier_walk);
	self.monsterinfo.run = SAVABLE(soldier_run);
	self.monsterinfo.attack = SAVABLE(soldier_attack);
	self.monsterinfo.sight = SAVABLE(soldier_sight);
	self.monsterinfo.reacttodamage = SAVABLE(soldier_reacttodamage);
#ifdef ROGUE_AI
	self.monsterinfo.dodge = SAVABLE(M_MonsterDodge);
	self.monsterinfo.blocked = SAVABLE(soldier_blocked);
	self.monsterinfo.duck = SAVABLE(soldier_duck);
	self.monsterinfo.unduck = SAVABLE(monster_duck_up);
	self.monsterinfo.sidestep = SAVABLE(soldier_sidestep);

	if(self.spawnflags & 8)	// blind
		self.monsterinfo.stand = SAVABLE(soldier_blind);
#else
	self.monsterinfo.dodge = SAVABLE(soldier_dodge);
#endif

	gi.linkentity(self);

	self.monsterinfo.stand(self);

	walkmonster_start(self);
}


/*QUAKED monster_soldier_light (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier_light(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	self.skinnum = 0;
	self.health = 20;
	self.gib_health = -30;

	SP_monster_soldier_x(self, "models/monsters/soldier/tris.md2");

	sound_pain_light = gi.soundindex("soldier/solpain2.wav");
	sound_death_light = gi.soundindex("soldier/soldeth2.wav");
	gi.modelindex("models/objects/laser/tris.md2");
	gi.soundindex("misc/lasfly.wav");
	gi.soundindex("soldier/solatck2.wav");

#ifdef ROGUE_AI
	self.monsterinfo.blindfire = true;
#endif
}

REGISTER_ENTITY(MONSTER_SOLDIER_LIGHT, monster_soldier_light);

/*QUAKED monster_soldier (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	self.skinnum = 2;
	self.health = 30;
	self.gib_health = -30;

	SP_monster_soldier_x(self, "models/monsters/soldier/tris.md2");

	sound_pain = gi.soundindex("soldier/solpain1.wav");
	sound_death = gi.soundindex("soldier/soldeth1.wav");
	gi.soundindex("soldier/solatck1.wav");
}

REGISTER_ENTITY(MONSTER_SOLDIER, monster_soldier);

/*QUAKED monster_soldier_ss (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier_ss(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	self.skinnum = 4;
	self.health = 40;
	self.gib_health = -30;

	SP_monster_soldier_x(self, "models/monsters/soldier/tris.md2");

	sound_pain_ss = gi.soundindex("soldier/solpain3.wav");
	sound_death_ss = gi.soundindex("soldier/soldeth3.wav");
	gi.soundindex("soldier/solatck3.wav");
}

REGISTER_ENTITY(MONSTER_SOLDIER_SS, monster_soldier_ss);

#ifdef THE_RECKONING
//
// SPAWN
//
inline void SP_monster_soldier_h(entity &self)
{
	SP_monster_soldier_x(self, "models/monsters/soldierh/tris.md2");
	soldierh_modelindex = self.modelindex;
}


/*QUAKED monster_soldier_ripper (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier_ripper(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict (self);
		return;
	}

	self.skinnum = 0;
	self.health = 50;
	self.gib_health = -30;

	SP_monster_soldier_h (self);

	sound_pain_light = gi.soundindex ("soldier/solpain2.wav");
	sound_death_light = gi.soundindex ("soldier/soldeth2.wav");
	
	gi.modelindex ("models/objects/boomrang/tris.md2");
	gi.soundindex ("misc/lasfly.wav");
	gi.soundindex ("soldier/solatck2.wav");

#ifdef ROGUE_AI
	self.monsterinfo.blindfire = true;
#endif
}

static REGISTER_ENTITY(MONSTER_SOLDIER_RIPPER, monster_soldier_ripper);

/*QUAKED monster_soldier_hypergun (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier_hypergun(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict (self);
		return;
	}

	self.skinnum = 2;
	self.health = 60;
	self.gib_health = -30;

	SP_monster_soldier_h (self);

	gi.modelindex ("models/objects/blaser/tris.md2");
	sound_pain = gi.soundindex ("soldier/solpain1.wav");
	sound_death = gi.soundindex ("soldier/soldeth1.wav");
	gi.soundindex ("soldier/solatck1.wav");

#ifdef ROGUE_AI
	self.monsterinfo.blindfire = true;
#endif
}

static REGISTER_ENTITY(MONSTER_SOLDIER_HYPERGUN, monster_soldier_hypergun);

/*QUAKED monster_soldier_lasergun (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_soldier_lasergun(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict (self);
		return;
	}

	self.skinnum = 4;
	self.health = 70;
	self.gib_health = -30;

	SP_monster_soldier_h (self);

	sound_pain_ss = gi.soundindex ("soldier/solpain3.wav");
	sound_death_ss = gi.soundindex ("soldier/soldeth3.wav");
	gi.soundindex ("soldier/solatck3.wav");
}

static REGISTER_ENTITY(MONSTER_SOLDIER_LASERGUN, monster_soldier_lasergun);
#endif
#endif
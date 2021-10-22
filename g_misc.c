﻿// g_misc.c

#include "g_local.h"
#include "c_botnav.h"

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal(edict_t* ent, edict_t* other, edict_t* activator)
{
	ent->count ^= 1;		// toggle state
//	gi.dprintf ("portalstate: %i = %i\n", ent->style, ent->count);
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(edict_t* ent)
{
	ent->classindex = FUNC_AREAPORTAL; // MrG{DRGN} 
	ent->use = Use_Areaportal;
	ent->count = 0;		// allways start closed;
}

//=====================================================

/*
=================
Misc functions
=================
*/
void VelocityForDamage(int damage, vec3_t v)
{
	v[0] = 100.0F * crandom(); /* MrG{DRGN} Explicit float */
	v[1] = 100.0F * crandom(); /* MrG{DRGN} Explicit float */
	v[2] = 200.0F + 100.0F * random(); /* MrG{DRGN} Explicit float */

	if (damage < 50)
		VectorScale(v, 0.7F, v);  /* MrG{DRGN} added  F, as this was causing truncation from double to float.*/
	else
		VectorScale(v, 1.2F, v);  /* MrG{DRGN} added  F, as this was causing truncation from double to float.*/
}

void ClipGibVelocity(edict_t* ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200;	// always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}

/*
=================
gibs
=================
*/

void gib_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	// MrG{DRGN} 
	if (!self || !other) /* plane can be NULL, surf is unused */
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	}
}

void gib_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowGib(edict_t* self, char* gibname, int damage, int type)
{
	edict_t* gib;
	vec3_t	vd;
	vec3_t	origin = { 0, 0, 0 };
	vec3_t	size;
	float	vscale;

	gib = G_Spawn();

	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel(gib, gibname);
	gib->classname = "gib";	//MATTHIAS
	gib->classindex = GIB;
	VectorSet(gib->mins, -4, -4, -4);// MrG{DRGN} 
	VectorSet(gib->maxs, 4, 4, 4);// MrG{DRGN} 
	gib->solid = SOLID_TRIGGER;// MrG{DRGN} 
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->health = 250; /* MrG{DRGN */
	gib->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		gib->classindex = GIB; // MrG{DRGN} 
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 1.0;
	}
	else
	{
		gib->classindex = GIB_METALLIC; // MrG{DRGN} 
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage * 2, vd);
	VectorMA(self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random() * 600;
	gib->avelocity[1] = random() * 600;
	gib->avelocity[2] = random() * 600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 2 + random() * 2;

	gi.linkentity(gib);
}

void ThrowClientHead(edict_t* self, int damage)
{
	vec3_t	vd;
	char* gibname;

	gibname = "models/objects/gibs/skull/tris.md2";
	self->s.skinnum = 0;

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	//self->classname = "gibhead"; // MrG{DRGN} 
	self->classindex = GIBHEAD; // MrG{DRGN} 
	VectorSet(self->mins, -16, -16, 0);
	VectorSet(self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage(damage, vd);
	VectorAdd(self->velocity, vd, self->velocity);

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else/* MrG{DRGN} NULL clients don't need to think */
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity(self);
}

/*
=================
debris
=================
*/
void debris_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowDebris(edict_t* self, char* modelname, float speed, vec3_t origin)
{
	edict_t* chunk;
	vec3_t	v = { 0 };
	chunk = G_Spawn();
	VectorCopy(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random() * 5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->classindex = DEBRIS; // MrG{DRGN} 
	chunk->takedamage = DAMAGE_YES;
	chunk->health = 250; /* MrG DRGN} */
	chunk->die = debris_die;
	gi.linkentity(chunk);
}

void BecomeExplosion1(edict_t* self)
{
	//ZOID
		//flags are important
	if (self->classindex == ITEM_FLAG_TEAM1)
	{
		CTFResetFlag(CTF_TEAM1); // this will free self!
		bprint_botsafe(PRINT_HIGH, "The %s flag has returned!\n",
			CTFTeamName(CTF_TEAM1));
		return;
	}
	if (self->classindex == ITEM_FLAG_TEAM2)
	{
		CTFResetFlag(CTF_TEAM2); // this will free self!
		bprint_botsafe(PRINT_HIGH, "The %s flag has returned!\n",
			CTFTeamName(CTF_TEAM1));
		return;
	}
	// techs are important too
	if (self->item && (self->item->flags & IT_TECH)) {
		CTFRespawnTech(self); // this frees self!
		return;
	}
	//ZOID

	if (Q_stricmp(self->classname, "rocket_turret") == 0
		|| Q_stricmp(self->classname, "laser_turret") == 0)
		numturrets--;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	// FWP fix for weapon killer bug - respawn weapon after showing cute explosion

	if (self->item && (self->item->flags & (IT_WEAPON | IT_POWERUP | IT_AMMO | IT_ARMOR)))
	{
		DoRespawn(self); // this frees self!
		return;
	}
	
	G_FreeEdict(self);
}

void BecomeExplosion2(edict_t* self)
{
	if (Q_stricmp(self->classname, "rocket_turret") == 0
		|| Q_stricmp(self->classname, "laser_turret") == 0)
		numturrets--;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

void path_corner_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	vec3_t		v = { 0 };
	edict_t* next;

	// MrG{DRGN} 
	if (!self || !other) /* plane, & surf are unused */
	{
		G_FreeEdict(self);
		return;
	}

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char* savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = NULL;

	if ((next) && (next->spawnflags & 1))
	{
		VectorCopy(next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy(v, other->s.origin);
		next = G_PickTarget(next->target);
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		//other->monsterinfo.pausetime = level.time + self->wait;
		//other->monsterinfo.stand (other);
		return;
	}

	if (!other->movetarget)
	{
		//other->monsterinfo.pausetime = level.time + 100000000;
		//other->monsterinfo.stand (other);
	}
	else
	{
		VectorSubtract(other->goalentity->s.origin, other->s.origin, v);
		other->ideal_yaw = vectoyaw(v);
	}
}

void SP_path_corner(edict_t* self)
{
	if (!self->targetname)
	{
		gi.dprintf("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->classindex = PATH_CORNER; // MrG{DRGN} 
	self->touch = path_corner_touch;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

void SP_point_combat(edict_t* self)
{
	self->classindex = POINT_COMBAT; // MrG{DRGN} 
	G_FreeEdict(self);
	return;
}

/*QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)
Just for the debugging level.  Don't use
*/
static int robotron[4];

void TH_viewthing(edict_t* ent)
{
	ent->s.frame = (ent->s.frame + 1) % 7;
	//	ent->s.frame = (ent->s.frame + 1) % 9;
	ent->nextthink = level.time + FRAMETIME;
	//	return;

	if (ent->spawnflags)
	{
		if (ent->s.frame == 0)
		{
			ent->spawnflags = (ent->spawnflags + 1) % 4 + 1;
			ent->s.modelindex = robotron[ent->spawnflags - 1];
		}
	}
}

void SP_viewthing(edict_t* ent)
{
	gi.dprintf("viewthing spawned\n");

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.renderfx = RF_FRAMELERP;
	VectorSet(ent->mins, -16, -16, -24);
	VectorSet(ent->maxs, 16, 16, 32);
	//	ent->s.modelindex = gi.modelindex ("models/player_y/tris.md2");
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->classindex = VIEWTHING; // MrG{DRGN} 
	gi.linkentity(ent);
	ent->nextthink = level.time + 0.5;
	ent->think = TH_viewthing;
	return;
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null(edict_t* self)
{
	self->classindex = INFO_NULL; // MrG{DRGN} 
	G_FreeEdict(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull(edict_t* self)
{
	self->classindex = INFO_NOTNULL; // MrG{DRGN} 
	VectorCopy(self->s.origin, self->absmin);
	VectorCopy(self->s.origin, self->absmax);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

#define START_OFF	1

static void light_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring(CS_LIGHTS + self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

void SP_light(edict_t* self)
{
	// no targeted lights in deathmatch, because they cause global messages
	/* MrG{DRGN} Chaos is always deathmatch */
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}
	self->classindex = LIGHT; // MrG{DRGN} 
	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS + self->style, "a");
		else
			gi.configstring(CS_LIGHTS + self->style, "m");
	}
}

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

void func_wall_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox(self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity(self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void SP_func_wall(edict_t* self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & 7) == 0)
	{
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1))
	{
		//		gi.dprintf("func_wall missing TRIGGER_SPAWN\n");
		self->spawnflags |= 1;
	}

	// yell if the spawnflags are odd
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->classindex = FUNC_WALL; // MrG{DRGN} 
	self->use = func_wall_use;
	if (self->spawnflags & 4)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity(self);
}

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	// only squash thing we fall on top of

	if (!self || !other || !plane)
		return;

	if (plane->normal[2] < 1.0)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;
	T_Damage(other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void func_object_release(edict_t* self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox(self);
	func_object_release(self);
}

void SP_func_object(edict_t* self)
{
	gi.setmodel(self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * FRAMETIME;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;
	self->classindex = FUNC_OBJECT; // MrG{DRGN} 
	gi.linkentity(self);
}

void SP_func_explosive(edict_t* self)
{
	self->classindex = FUNC_EXPLOSIVE; // MrG{DRGN} 
	G_FreeEdict(self);
	return;
}

void SP_misc_explobox(edict_t* self)
{
	self->classindex = MISC_EXPLOBOX; // MrG{DRGN} 
	G_FreeEdict(self);
	return;
}

//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
*/

void misc_blackhole_use(edict_t* ent, edict_t* other, edict_t* activator)
{
	G_FreeEdict(ent);
}

void misc_blackhole_think(edict_t* self)
{
	if (++self->s.frame < 19)
		self->nextthink = level.time + FRAMETIME;
	else
	{
		self->s.frame = 0;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_blackhole(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	VectorSet(ent->mins, -64, -64, 0);
	VectorSet(ent->maxs, 64, 64, 8);
	ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->classindex = MISC_BLACKHOLE; // MrG{DRGN} 
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
*/

void misc_eastertank_think(edict_t* self)
{
	if (++self->s.frame < 293)
		self->nextthink = level.time + FRAMETIME;
	else
	{
		self->s.frame = 254;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_eastertank(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, -16);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent->classindex = MISC_EASTERTANK; // MrG{DRGN} 
	ent->s.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
*/

void misc_easterchick_think(edict_t* self)
{
	if (++self->s.frame < 247)
		self->nextthink = level.time + FRAMETIME;
	else
	{
		self->s.frame = 208;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_easterchick(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, 0);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->classindex = MISC_EASTERCHICK; // MrG{DRGN} 
	ent->s.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
*/

void misc_easterchick2_think(edict_t* self)
{
	if (++self->s.frame < 287)
		self->nextthink = level.time + FRAMETIME;
	else
	{
		self->s.frame = 248;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_easterchick2(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -32, -32, 0);
	VectorSet(ent->maxs, 32, 32, 32);
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->classindex = MISC_EASTERCHICK2; // MrG{DRGN} 
	ent->s.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity(ent);
}

/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

void commander_body_think(edict_t* self)
{
	if (++self->s.frame < 24)
		self->nextthink = level.time + FRAMETIME;
	else
		self->nextthink = 0;

	if (self->s.frame == 22)
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1, ATTN_NORM, 0);
}

void commander_body_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->think = commander_body_think;
	self->nextthink = level.time + FRAMETIME;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

void commander_body_drop(edict_t* self)
{
	self->movetype = MOVETYPE_TOSS;
	self->s.origin[2] += 2;
}

void SP_monster_commander_body(edict_t* self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->classindex = MONSTER_COMMANDER_BODY; // MrG{DRGN} 
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet(self->mins, -32, -32, 0);
	VectorSet(self->maxs, 32, 32, 48);
	self->use = commander_body_use;
	self->takedamage = DAMAGE_YES;
	self->flags = FL_GODMODE;
	self->s.renderfx |= RF_FRAMELERP;
	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self->think = commander_body_drop;
	self->nextthink = level.time + 5 * FRAMETIME;
}

/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
*/
void misc_banner_think(edict_t* ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_banner(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->classindex = MISC_BANNER; // MrG{DRGN} 
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->s.frame = rand() % 16;
	gi.linkentity(ent);

	ent->think = misc_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_deadsoldier(edict_t* ent)
{
	ent->classindex = MISC_DEADSOLDIER; // MrG{DRGN} 
	G_FreeEdict(ent);
	return;
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast the Viper should fly
*/

extern void train_use(edict_t* self, edict_t* other, edict_t* activator);
extern void func_train_find(edict_t* self);

void misc_viper_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_viper(edict_t* ent)
{
	if (!ent->target)
	{
		gi.dprintf("misc_viper without a target at %s\n", vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	ent->classindex = MISC_VIPER; // MrG{DRGN} 
	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_viper_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -176, -120, -24);
	VectorSet(ent->maxs, 176, 120, 72);
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	ent->classindex = MISC_BIGVIPER; // MrG{DRGN} 
	gi.linkentity(ent);
}

/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"	how much boom should the bomb make?
*/
void misc_viper_bomb_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	// MrG{DRGN} 
	if (!self) /* other, plane, & surf are unused */
	{
		G_FreeEdict(self);
		return;
	}

	G_UseTargets(self, self->activator);

	self->s.origin[2] = self->absmin[2] + 1;
	T_RadiusDamage(self, self, self->dmg, NULL, self->dmg + 40, MOD_BOMB);
	BecomeExplosion2(self);
}

void misc_viper_bomb_prethink(edict_t* self)
{
	vec3_t	v;
	float	diff;

	self->groundentity = NULL;

	diff = self->timestamp - level.time;
	if (diff < -1.0)
		diff = -1.0;

	VectorScale(self->moveinfo.dir, 1.0 + diff, v);
	v[2] = diff;

	diff = self->s.angles[2];
	vectoangles(v, self->s.angles);
	self->s.angles[2] = diff + 10;
}

void misc_viper_bomb_use(edict_t* self, edict_t* other, edict_t* activator)
{
	edict_t* viper;

	self->solid = SOLID_BBOX;
	self->svflags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = NULL;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;

	viper = G_Find(NULL, FOFS(classname), "misc_viper");
	VectorScale(viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);

	self->timestamp = level.time;
	VectorCopy(viper->moveinfo.dir, self->moveinfo.dir);
}

void SP_misc_viper_bomb(edict_t* self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");
	self->classindex = MISC_VIPER_BOMB; // MrG{DRGN} 
	if (!self->dmg)
		self->dmg = 1000;

	self->use = misc_viper_bomb_use;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast it should fly
*/

extern void train_use(edict_t* self, edict_t* other, edict_t* activator);
extern void func_train_find(edict_t* self);

void misc_strogg_ship_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_strogg_ship(edict_t* ent)
{
	if (!ent->target)
	{
		gi.dprintf("%s without a target at %s\n", ent->classname, vtos(ent->absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->classindex = MISC_STROGG_SHIP; // MrG{DRGN} 
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	VectorSet(ent->mins, -16, -16, 0);
	VectorSet(ent->maxs, 16, 16, 32);

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = misc_strogg_ship_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
*/
void misc_satellite_dish_think(edict_t* self)
{
	self->s.frame++;
	if (self->s.frame < 38)
		self->nextthink = level.time + FRAMETIME;
}

void misc_satellite_dish_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_misc_satellite_dish(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -64, -64, 0);
	VectorSet(ent->maxs, 64, 64, 128);
	ent->classindex = MISC_SATELLITE_DISH; // MrG{DRGN} 
	ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.linkentity(ent);
}

/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine1(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->classindex = LIGHT_MINE1; // MrG{DRGN} 
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine2(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->classindex = LIGHT_MINE2; // MrG{DRGN} 
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->classindex = MISC_GIB_ARM; // MrG{DRGN} 
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->classindex = MISC_GIB_LEG; // MrG{DRGN} 
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->classindex = MISC_GIB_HEAD; // MrG{DRGN} 
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(edict_t* self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->classindex = TARGET_CHARACTER; // MrG{DRGN} 
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity(self);
	return;
}

/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/

void target_string_use(edict_t* self, edict_t* other, edict_t* activator)
{
	edict_t* e;
	int	n;
	int	l;
	char	c;

	l = (int)strlen(self->message); /* MrG{DRGN} avoid possible loss of data warning */
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string(edict_t* self)
{
	if (!self->message)
		self->message = "";
	self->classindex = TARGET_STRING; // MrG{DRGN} 
	self->use = target_string_use;
}

/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

#define	CLOCK_MESSAGE_SIZE	16

// don't let field width of any clock messages change, or it
// could cause an overwrite after a game load

static void func_clock_reset(edict_t* self)
{
	self->activator = NULL;
	if (self->spawnflags & 1)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown(edict_t* self)
{
	if (self->style == 0)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		return;
	}

	if (self->style == 2)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
		return;
	}
}

void func_clock_think(edict_t* self)
{
	if (!self->enemy)
	{
		self->enemy = G_Find(NULL, FOFS(targetname), self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags & 1)
	{
		func_clock_format_countdown(self);
		self->health++;
	}
	else if (self->spawnflags & 2)
	{
		func_clock_format_countdown(self);
		self->health--;
	}
	else
	{
		struct tm* ltime;
		time_t		gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use(self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) ||
		((self->spawnflags & 2) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char* savetarget;
			char* savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets(self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8))
			return;

		func_clock_reset(self);

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1;
}

void func_clock_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (!(self->spawnflags & 8))
		self->use = NULL;
	if (self->activator)
		return;
	self->activator = activator;
	self->think(self);
}

void SP_func_clock(edict_t* self)
{
	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count))
	{
		gi.dprintf("%s with no count at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->classindex = FUNC_CLOCK; // MrG{DRGN} 
	if ((self->spawnflags & 1) && (!self->count))
		self->count = 60 * 60;

	func_clock_reset(self);

	self->message = gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = func_clock_think;

	if (self->spawnflags & 4)
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1;
}

//=================================================================================

void teleporter_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	edict_t* dest;
	int			i;
	vec3_t		nodeo = { 0, 0, 0 };

	// MrG{DRGN} 
	if (!self || !other) /* plane &, surf are unused */
	{
		return;
	}

	if (!other->client && !tele_fire->value)
		return;
	dest = G_Find(NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf("Couldn't find destination\n");
		return;
	}
	if (other->client)
	{
		if (!Bot_FindNode(self, 120, TELEPORT_NODE)
			&& dntg->value)
		{
			//start node
			VectorCopy(other->s.origin, nodeo);

			if (!(other->client->ps.pmove.pm_flags & PMF_DUCKED))
			{
				nodeo[2] += 5;
				Bot_PlaceNode(nodeo, TELEPORT_NODE, 0);
			}
			else
				Bot_PlaceNode(nodeo, TELEPORT_NODE, 1);

			Bot_CalcNode(other, numnodes);

			//dest node
			VectorCopy(dest->s.origin, nodeo);
			nodeo[2] += 20;
			Bot_PlaceNode(nodeo, NORMAL_NODE, 0);
			Bot_CalcNode(other, numnodes);

			//connection
			nodes[numnodes - 1].dist[numnodes] = 1;

			nprintf(PRINT_HIGH, "Teleporter nodes placed and connected!\n");
		}
	}
	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	if (other->client)
	{
		VectorClear(other->velocity);
		// set angles
		other->client->ps.pmove.pm_time = 160 >> 3;		// hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	}
	// draw the teleport splash at source and on the player
	self->owner->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;
	if (other->client)
	{
		for (i = 0; i < 3; i++)
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);

		VectorClear(other->client->ps.viewangles);
		VectorClear(other->client->v_angle);
	}
	VectorClear(other->s.angles);
	// kill anything at the destination
	KillBox(other);

	gi.linkentity(other);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter(edict_t* ent)
{
	edict_t* trig;

	if (!ent->target)
	{
		gi.dprintf("teleporter without a target.\n");
		G_FreeEdict(ent);
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->classindex = MISC_TELEPORTER; // MrG{DRGN} 
	ent->s.skinnum = 1;
	ent->s.effects = EF_TELEPORTER;
	ent->s.sound = gi.soundindex("world/amb10.wav");
	ent->solid = SOLID_BBOX;

	VectorSet(ent->mins, -32, -32, -24);
	VectorSet(ent->maxs, 32, 32, -16);
	gi.linkentity(ent);

	trig = G_Spawn();
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	VectorCopy(ent->s.origin, trig->s.origin);
	VectorSet(trig->mins, -8, -8, 8);
	VectorSet(trig->maxs, 8, 8, 24);
	gi.linkentity(trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->classindex = MISC_TELEPORTER_DEST; // MrG{DRGN} 
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	//	ent->s.effects |= EF_FLIES;
	VectorSet(ent->mins, -32, -32, -24);
	VectorSet(ent->maxs, 32, 32, -16);
	gi.linkentity(ent);
}

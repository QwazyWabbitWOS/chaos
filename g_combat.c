// g_combat.c
#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage(edict_t* target, edict_t* inflictor)
{
	vec3_t	dest = { 0 };
	trace_t	trace;

	// bmodels need special checking because their origin is 0,0,0
	if (target->movetype == MOVETYPE_PUSH)
	{
		VectorAdd(target->absmin, target->absmax, dest);
		VectorScale(dest, 0.5, dest);
		trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == target)
			return true;
		return false;
	}

	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, target->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	return false;
}

/*
============
Killed
============
*/
void Killed(edict_t* target, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	if (target->health < -999)
		target->health = -999;

	target->enemy = attacker;

	if (target->movetype == MOVETYPE_PUSH || target->movetype == MOVETYPE_STOP || target->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		target->die(target, inflictor, attacker, damage, point);
		return;
	}

	target->die(target, inflictor, attacker, damage, point);
}

/*
================
SpawnDamage
================
*/
void SpawnDamage(int type, vec3_t origin, vec3_t normal, int damage)
{
	/* Unused outside of function
	if (damage > 255)
		damage = 255; */
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	//	gi.WriteByte (damage);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
============
T_Damage

target		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage target
	example: target=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against target as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckPowerArmor(edict_t* ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t* client;
	int			save;
	int			power_armor_type;
	size_t			index = 0;/* MrG{+DRGN} initialized and changed to size_t */
	int			damagePerCell;
	int			pa_te_type;
	int			power = 0;/* MrG{+DRGN} initialized 09/23/2020 */
	int			power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType(ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(it_cells);
			power = client->pers.inventory[index];
		}
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec;
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors(ent->s.angles, forward, NULL, NULL);
		VectorSubtract(point, ent->s.origin, vec);
		VectorNormalize(vec);
		dot = DotProduct(vec, forward);
		if (dot <= 0.3)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 1; // power armor is weaker in CTF
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;
	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage(pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 0.2F;

	power_used = save / damagePerCell;

	//if (client) MrG{DRGN} if there isn't a client we returned 0 above
		client->pers.inventory[index] -= power_used;

	return save;
}

static int CheckArmor(edict_t* ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t* client;
	int			save;
	int			index;
	gitem_t* armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	index = ArmorIndex(ent);
	if (!index)
		return 0;

	armor = GetItemByIndex(index);

	if (dflags & DAMAGE_ENERGY)
		save = (int)ceilf(((gitem_armor_t*)armor->info)->energy_protection * damage); /* MrG{DRGN} safe cast */
	else
		save = (int)ceilf(((gitem_armor_t*)armor->info)->normal_protection * damage);  /* MrG{DRGN} safe cast */
	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage(te_sparks, point, normal, save);

	return save;
}

qboolean CheckTeamDamage(edict_t* target, edict_t* attacker)
{
	//ZOID
	if (ctf->value && target->client && attacker->client)
		if (target->client->resp.ctf_team == attacker->client->resp.ctf_team &&
			target != attacker)
			return true;
	//ZOID

			//FIXME make the next line real and uncomment this block
			// if ((ability to damage a teammate == OFF) && (target's team == attacker's team))
	return false;
}

void T_Damage(edict_t* target, edict_t* inflictor, edict_t* attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t* client;
	int			take;
	int			save;
	int			asave;
	int			psave;
	int			te_sparks;

	if (!target->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((target != attacker) && ((deathmatch->value && ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) || coop->value))
	{
		if (OnSameTeam(target, attacker))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
	}
	meansOfDeath = mod;

	client = target->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

	//ZOID
	//strength tech
	damage = CTFApplyStrength(attacker, damage);
	//ZOID

	if (target->flags & FL_NO_KNOCKBACK)
		knockback = 0;

	// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (target->movetype != MOVETYPE_NONE) && (target->movetype != MOVETYPE_BOUNCE) && (target->movetype != MOVETYPE_PUSH) && (target->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			int	mass;/* MrG{DRGN this should have been int, not float */

			if (target->mass < 50)
				mass = 50;
			else
				mass = target->mass;

			if (target->client && attacker == target)
				VectorScale(dir, 1600.0F * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale(dir, 500.0F * (float)knockback / mass, kvel);

			VectorAdd(target->velocity, kvel, target->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ((target->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;
		SpawnDamage(te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (target->pain_debounce_time < level.time)
		{
			gi.sound(target, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			target->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}

	//ZOID
	//team armor protect
	if (ctf->value && target->client && attacker->client &&
		target->client->resp.ctf_team == attacker->client->resp.ctf_team &&
		target != attacker && ((int)dmflags->value & DF_ARMOR_PROTECT)) {
		psave = asave = 0;
	}
	else {
		//ZOID
		psave = CheckPowerArmor(target, point, normal, take, dflags);
		take -= psave;

		asave = CheckArmor(target, point, normal, take, te_sparks, dflags);
		take -= asave;
	}

	//treat cheat/powerup savings the same as armor
	asave += save;

	//ZOID
	//resistance tech
	take = CTFApplyResistance(target, take);
	//ZOID

		// team damage avoidance
	if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage(target, attacker))
		return;

	//ZOID
	CTFCheckHurtCarrier(target, attacker);
	//ZOID

	// do the damage
	if (take)
	{
		if ((target->svflags & SVF_MONSTER) || (client))
			SpawnDamage(TE_BLOOD, point, normal, take);
		else
			SpawnDamage(te_sparks, point, normal, take);

		if (!CTFMatchSetup())
			target->health = target->health - take;

		if (target->health <= 0)
		{
			if ((target->svflags & SVF_MONSTER) || (client))
				target->flags |= FL_NO_KNOCKBACK;
			Killed(target, inflictor, attacker, take, point);
			return;
		}
	}

	if (client)
	{
		if (!(target->flags & FL_GODMODE) && (take) && !CTFMatchSetup())
			target->pain(target, attacker, knockback, take);
	}
	else if (take)
	{
		if (target->pain)
			target->pain(target, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy(point, client->damage_from);
	}
}

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage(edict_t* inflictor, edict_t* attacker, float damage, edict_t* ignore, float radius, int mod)
{
	float	points;
	edict_t* ent = NULL;
	vec3_t	v = { 0 };
	vec3_t	dir = { 0 };

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd(ent->mins, ent->maxs, v);
		VectorMA(ent->s.origin, 0.5, v, v);
		VectorSubtract(inflictor->s.origin, v, v);
		points = damage - 0.5F * VectorLength(v);
		if (ent == attacker)
			points = points * 0.5F;
		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				VectorSubtract(ent->s.origin, inflictor->s.origin, dir);
				T_Damage(ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

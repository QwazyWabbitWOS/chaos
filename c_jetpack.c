#include "g_local.h"

void JetpackInitVars(void)
{
#ifdef	CHAOS_RETAIL
	ban_jetpack = gi.cvar("ban_jetpack", "1", CVAR_LATCH);
#else
	ban_jetpack = gi.cvar("ban_jetpack", "0", CVAR_LATCH);
#endif
	start_jetpack = gi.cvar("start_jetpack", "0", CVAR_LATCH);
}

void Use_Jet(edict_t* ent, gitem_t* item)
{
#ifdef	CHAOS_RETAIL
	cprint_botsafe(ent, PRINT_HIGH, "The Jetpack has been deactivated in the retail version!\n");
	return;
#endif

	if (!ent)
	{
		return;
	}

	ValidateSelectedItem(ent);

	if (ent->client->jet_remaining == 0)
		ent->client->jet_remaining = 600;

	if (Jet_Active(ent) && ((int)dmflags->value & DF_INSTANT_JET))
		return;
	else if (Jet_Active(ent))
		ent->client->jet_framenum = 0;
	else
		ent->client->jet_framenum = level.framenum + ent->client->jet_remaining;
}

qboolean Jet_AvoidGround(edict_t* ent)
{
	vec3_t        new_origin = { 0 };
	trace_t       trace;
	qboolean      success;

	if (!ent)
	{
		return false;
	}

	/*Check if there is enough room above us before we change origin[2]*/
	new_origin[0] = ent->s.origin[0];
	new_origin[1] = ent->s.origin[1];
	new_origin[2] = ent->s.origin[2] + 0.5F;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_MONSTERSOLID);

	if ((success = ((trace.plane.normal[2]) == 0)) != 0)
		ent->s.origin[2] += 0.5;

	return success;
}

qboolean Jet_Active(edict_t* ent)
{
	if (!ent)
	{
		return false;
	}

	return (ent->client->jet_framenum >= level.framenum);
}

void Jet_BecomeExplosion(edict_t* ent, int damage)
{
	if (!ent)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	gi.sound(ent, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

	ThrowGib(ent, "models/objects/gibs/sm_gib1/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/sm_gib2/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/sm_gib2/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/sm_gib3/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/sm_gib4/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/sm_gib5/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(ent, "models/objects/gibs/bone2/tris.md2", damage, GIB_ORGANIC);

	ThrowClientHead(ent, damage);
	ent->takedamage = DAMAGE_NO;
}

void Jet_ApplyLifting(edict_t* ent)
{
	float         delta;
	vec3_t        new_origin = { 0 };
	trace_t       trace;
	int           time = 24;
	float         amplitude = 2.0;

	if (!ent)
	{
		return;
	}

	delta = sinf((float)((level.framenum % time) * (360.0f / time)) / 180 * M_PI) * amplitude;/* MrG{DRGN} use sinf not sin to calculate the delta, since it's a float */
	delta = (float)((int)(delta * 8)) / 8;

	VectorCopy(ent->s.origin, new_origin);
	new_origin[2] += delta;

	/*(if(VectorLength(ent->velocity) == 0)
	{
	  new_origin[0] -= 0.125;
	  new_origin[1] -= 0.125;
	  new_origin[2] -= 0.125;
	}*/

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_MONSTERSOLID);
	if (trace.plane.normal[2] == 0)
		VectorCopy(new_origin, ent->s.origin);
}

void Jet_ApplySparks(edict_t* ent)
{
	vec3_t  forward, right;
	vec3_t  pack_pos, jet_vector;

	if (!ent)
	{
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -7, pack_pos);
	VectorAdd(pack_pos, ent->s.origin, pack_pos);
	pack_pos[2] += (ent->viewheight);
	VectorScale(forward, -50, jet_vector);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(pack_pos);
	gi.WriteDir(jet_vector);
	gi.multicast(pack_pos, MULTICAST_PVS);

	pack_pos[2] -= 5;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(pack_pos);
	gi.WriteDir(jet_vector);
	gi.multicast(pack_pos, MULTICAST_PVS);
}

void Jet_ApplyRolling(edict_t* ent, const vec3_t right)
{
	float roll, value = 0.05F, sign = -1;

	if (!ent)
	{
		return;
	}

	roll = DotProduct(ent->velocity, right) * value * sign;
	ent->client->kick_angles[ROLL] = roll;
}

void Jet_ApplyJet(edict_t* ent, usercmd_t* ucmd)
{
	static float direction;
	vec3_t acc = { 0 };
	vec3_t forward, right;
	int    i;

	if (!ent)
	{
		return;
	}

	ent->client->ps.pmove.gravity = 0;
	AngleVectors(ent->client->v_angle, forward, right, NULL);

	if (ent->client->jet_next_think <= level.framenum)
	{
		ent->client->jet_next_think = level.framenum + 1;
		VectorClear(acc);

		if (ucmd->forwardmove)
		{
			direction = (ucmd->forwardmove < 0) ? -1.0 : 1.0;
			acc[0] += direction * forward[0] * 60;
			acc[1] += direction * forward[1] * 60;
			acc[2] += direction * forward[2] * 60;
		}

		if (ucmd->sidemove)
		{
			direction = (ucmd->sidemove < 0) ? -1.0 : 1.0;
			acc[0] += right[0] * direction * 40;
			acc[1] += right[1] * direction * 40;
		}

		if (ucmd->upmove)
			acc[2] += ucmd->upmove > 0 ? 30 : -30;

		ent->velocity[0] += -(ent->velocity[0] / 6.0);
		ent->velocity[1] += -(ent->velocity[1] / 6.0);
		ent->velocity[2] += -(ent->velocity[2] / 7.0);
		VectorAdd(ent->velocity, acc, ent->velocity);

		ent->velocity[0] = (float)((int)(ent->velocity[0] * 8)) / 8;
		ent->velocity[1] = (float)((int)(ent->velocity[1] * 8)) / 8;
		ent->velocity[2] = (float)((int)(ent->velocity[2] * 8)) / 8;

		for (i = 0; i < 2; i++) /*allow z-velocity to be greater*/
		{
			if (ent->velocity[i] > 300)
				ent->velocity[i] = 300;
			else if (ent->velocity[i] < -300)
				ent->velocity[i] = -300;
		}

		if (VectorLength(acc) == 0)
			Jet_ApplyLifting(ent);
	}

	Jet_ApplyRolling(ent, right);
	Jet_ApplySparks(ent);
}

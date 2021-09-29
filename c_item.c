﻿#include "g_local.h"


void Use_Invisibility(edict_t* ent, gitem_t* item)
{
	/* MrG{DRGN} sanity check*/
	if (!ent || !item)
	{
		return;
	}
	/* END */

	ValidateSelectedItem(ent);

	ent->client->pers.inventory[ITEM_INDEX(item)]--;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/invis.wav"), 0.8F, ATTN_NORM, 0);/* MrG{DRGN} added  F, as this was causing truncation from double to float.*/
	ent->client->invisible = 1;
	ent->s.modelindex = REMOVED_MODEL;

	if (ent->client->invisible_framenum > level.framenum)
		ent->client->invisible_framenum += 300;
	else
		ent->client->invisible_framenum = level.framenum + 300.0F; /* MrG{DRGN} explicit float */

	ent->s.event = EV_PLAYER_TELEPORT;
}

void Use_Jet(edict_t* ent, gitem_t* item)
{
#ifdef	CHAOS_RETAIL
	cprintf2(ent, PRINT_HIGH, "The Jetpack has been deactivated in the retail version!\n");
	return;
#endif
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */
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
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return false;
	}
	/* END */

	/*Check if there is enough room above us before we change origin[2]*/
	new_origin[0] = ent->s.origin[0];
	new_origin[1] = ent->s.origin[1];
	new_origin[2] = ent->s.origin[2] + 0.5F; /* MrG{DRGN} explicit float */
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_MONSTERSOLID);

	if ((success = ((trace.plane.normal[2]) == 0)) != 0)
		ent->s.origin[2] += 0.5;

	return success;
}

qboolean Jet_Active(edict_t* ent)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return false;
	}
	/* END */

	return (ent->client->jet_framenum >= level.framenum);
}

void Jet_BecomeExplosion(edict_t* ent, int damage)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

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

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

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

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

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

void Jet_ApplyRolling(edict_t* ent, vec3_t right)
{
	float roll, value = 0.05F, sign = -1; /* MrG{DRGN} added  F, as this was causing truncation from double to float.*/

										  /* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	roll = DotProduct(ent->velocity, right) * value * sign;
	ent->client->kick_angles[ROLL] = roll;
}

void Jet_ApplyJet(edict_t* ent, usercmd_t* ucmd)
{
	static float direction;
	vec3_t acc = { 0 };
	vec3_t forward, right;
	int    i;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

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

void ShowScanner(edict_t* ent, char* layout)
{

	edict_t* player = g_edicts;
	int     i;
	char    stats[64] = { 0 }; /* MrG{DRGN} initialized */
	vec3_t  v = { 0 };

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	/* MrG{DRGN this is probably faster than the long series of if else statemnts that was here */
	switch (ent->client->scanneractive)
	{
	case 1:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan01");
		break;
	case 2:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan02");
		break;
	case 3:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan03");
		break;
	case 4:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan04");
		break;
	case 5:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan05");
		break;
	case 6:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan06");
		break;
	case 7:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan07");
		break;
	case 8:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan08");
		break;
	case 9:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan09");
		break;
	case 10:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan10");
		break;
	case 11:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan11");
		break;
	case 12:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan12");
		break;
	case 13:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan13");
		break;
	case 14:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan14");
		break;
	case 15:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan15");
		break;
	case 16:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan16");
		break;
	case 17:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan17");
		break;
	case 18:
	default:
		Com_sprintf(stats, sizeof(stats), "xr -120 yt 0 picn %s ", "scanner/scan18");
	}

	ent->client->scanneractive += 1;

	if (ent->client->scanneractive >= 19)
		ent->client->scanneractive = 1;

	// Main scanner graphic draw
	SAFE_STRCAT(layout, stats, LAYOUT_MAX_LENGTH);

	// Player's dots
	for (i = 0; i < game.maxclients; i++)
	{
		float	len;
		int		hd;

		// move to player edict
		player++;

		// in use
		if (!player->inuse || !player->client || (player == ent) || (player->health <= 0))
			continue;

		if (player->client->fakedeath == 1)
			continue;

		// calc player to enemy vector
		VectorSubtract(ent->s.origin, player->s.origin, v);

		// save height differential
		hd = v[2] / SCANNER_UNIT;

		// remove height component
		v[2] = 0;

		// calc length of distance from top-down view (no z)
		len = VectorLength(v) / SCANNER_UNIT;

		// in range ?
		if (len <= SCANNER_RANGE)
		{
			int		sx, sy;
			vec3_t  dp;
			vec3_t  normal = { 0,0,-1 };

			// normal vector to enemy
			VectorNormalize(v);

			// rotate round player view angle (yaw)
			RotatePointAroundVector(dp, normal, v, ent->s.angles[1]);

			// scale to fit scanner range (60 = pixel range of scanner)
			VectorScale(dp, len * 60 / SCANNER_RANGE, dp);

			// calc screen (x,y) (2 = half dot width)
			sx = (-60 + dp[1]) - 2;
			sy = (60 + dp[0]) - 2;

			if (hd > 0)
				Com_sprintf(stats, sizeof(stats), "xr %i yt %i picn %s ", sx, sy, "scanner/down");
			else if (hd < 0)
				Com_sprintf(stats, sizeof(stats), "xr %i yt %i picn %s ", sx, sy, "scanner/up");
			else
				Com_sprintf(stats, sizeof(stats), "xr %i yt %i picn %s ", sx, sy, "scanner/dot");

			SAFE_STRCAT(layout, stats, LAYOUT_MAX_LENGTH);

			// clear stats
			*stats = 0;
		}
	}
}

/* MrG{DRGN} moved here, since it's a Chaos Item! */
void Toggle_Scanner(edict_t* ent)
{
	if (!ent->client)
		return;
	if (ent->health <= 0)
		return;

	if (ent->client->scanneractive <= 0)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] == 0)
		{
			cprintf2(ent, PRINT_HIGH, "You don't have enough cells to run your scanner!\n");
			return;
		}
		cprintf2(ent, PRINT_HIGH, "Scanner ON\n");
		ent->client->scanneractive = 1;
		ent->client->showinventory = 0;
		ent->client->showscores = 0;
	}
	else
	{
		ent->client->scanneractive = 0;
		cprintf2(ent, PRINT_HIGH, "Scanner OFF\n");
	}
}
//SCANNER
void Scanner_Think(edict_t* ent)
{
	gclient_t* client;

	if (!ent || !ent->client)
		return;
	if (ent->health <= 0)
		return;
	client = ent->client;
	if ((level.time > client->nextscannercell) && (client->scanneractive > 0))
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] <= 0)
		{
			cprintf2(ent, PRINT_HIGH, "You don't have enough cells to run your scanner!\n");
			client->scanneractive = 0;
			client->nextscannercell = level.time;
		}
		else
		{
			if (ent->waterlevel >= 3)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/sonar.wav"), 1, ATTN_IDLE, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/radar.wav"), 1, ATTN_IDLE, 0);
			ent->client->pers.inventory[ITEM_INDEX(it_cells)]--;
			client->nextscannercell = level.time + 2;
		}
	}
}
/* MrG{DRGN} this is exactly the same as P_ProjectSoruce which was already global! */
/*
void P_ProjectSource2(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == RIGHT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}
*/

void Grapple_Reset(edict_t* ent)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	ent->owner->client->grapple_state = GRAPPLE_OFF;
	gi.sound(ent->owner, CHAN_AUTO, gi.soundindex("misc/grapple/reset.wav"), 1, ATTN_NORM, 0);
	G_FreeEdict(ent);
}

void Grapple_DrawCable(edict_t* ent)
{
	vec3_t	offset = { 0 };
	vec3_t	start;
	vec3_t	end = { 0 };
	vec3_t	f, r;
	vec3_t	dir = { 0 };
	float	distance;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	AngleVectors(ent->owner->client->v_angle, f, r, NULL);
	VectorSet(offset, 16, 16, ent->owner->viewheight - 8);
	P_ProjectSource(ent->owner->client, ent->owner->s.origin, offset, f, r, start);

	VectorSubtract(start, ent->owner->s.origin, offset);

	VectorSubtract(start, ent->s.origin, dir);
	distance = VectorLength(dir);
	// don't draw cable if close
	if (distance < 64)
		return;

	VectorCopy(ent->s.origin, end);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GRAPPLE_CABLE);
	gi.WriteShort(ent->owner - g_edicts);
	gi.WritePosition(ent->owner->s.origin);
	gi.WritePosition(end);
	gi.WritePosition(offset);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

void Grapple_Touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	vec3_t	offset = { 0 };
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	chainvec;		// chain's vector

	/* MrG{DRGN}*/
	if (!ent || !other) /* plane is unused, surf can be NULL */
	{
		G_FreeEdict(ent);
		return;
	}
	/* END */

	if (surf && (surf->flags & SURF_SKY))
	{
		Grapple_Reset(ent);
		return;
	}

	if (other->takedamage)
	{
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 100, 0, MOD_GRAPPLE);

		Grapple_Reset(ent);
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SHOTGUN);
	gi.WritePosition(ent->s.origin);
	gi.WriteDir(vec3_origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	// derive start point of chain
	AngleVectors(ent->owner->client->v_angle, forward, right, NULL);
	VectorSet(offset, 8, 8, ent->owner->viewheight - 8);
	P_ProjectSource(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

	// member angle is used to store the length of the chain
	_VectorSubtract(ent->s.origin, start, chainvec);
	ent->angle = VectorLength(chainvec);

	VectorCopy(other->velocity, ent->velocity);
	ent->owner->client->grapple_state = GRAPPLE_STARTSHRINK;	//Start with shrink

	ent->enemy = other;
	ent->touch = NULL;
	ent->nextthink = level.time + FRAMETIME;
	gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/hit.wav"), 1, ATTN_NORM, 0);
}

void Grapple_Think(edict_t* ent)
{
	vec3_t	offset = { 0 };
	vec3_t	start;
	vec3_t	dir = { 0 };
	vec3_t	f, r, playerv;
	vec_t len;
	float f1, f2;			// restrainment forces

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	if (ent->owner->client->grapple_state > 1)	// Grapple is attached
	{
		if ((!ent->enemy) || (ent->enemy->solid == SOLID_NOT) || (ent->owner->deadflag) || (ent->owner->s.event == EV_PLAYER_TELEPORT))
		{
			Grapple_Reset(ent);
			return;
		}

		VectorCopy(ent->enemy->velocity, ent->velocity);

		// auto shrink after launch
		if (ent->owner->client->grapple_state == GRAPPLE_STARTSHRINK && ent->angle > 40)
		{
			ent->angle -= 80;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/shrink.wav"), 1, ATTN_NORM, 0);
			if (ent->angle < 40)
			{
				ent->angle = 40;
				ent->owner->client->grapple_state = GRAPPLE_ATTACHED;
			}
		}

		// derive start point of chain
		AngleVectors(ent->owner->client->v_angle, f, r, NULL);
		VectorSet(offset, 8, 8, ent->owner->viewheight - 8);
		P_ProjectSource(ent->owner->client, ent->owner->s.origin, offset, f, r, start);

		// get info about chain
		VectorSubtract(ent->s.origin, start, dir);
		len = VectorLength(dir);

		if (len > ent->angle)	//out of current chainlen
		{
			VectorScale(dir, DotProduct(ent->owner->velocity, dir) / DotProduct(dir, dir), playerv);

			// restrainment default force
			f2 = (len - ent->angle) * 5;

			// if player's velocity heading is away from the hook
			if (DotProduct(ent->owner->velocity, dir) < 0)
			{
				if (len > ent->angle + 10)
					// remove player's velocity component moving away from hook
					VectorSubtract(ent->owner->velocity, playerv, ent->owner->velocity);
				f1 = f2;
			}
			else  // if player's velocity heading is towards the hook
			{
				if (VectorLength(playerv) < f2)
					f1 = f2 - VectorLength(playerv);
				else
					f1 = 0;
			}
		}
		else
			f1 = 0;

		VectorNormalize(dir);
		VectorMA(ent->owner->velocity, f1, dir, ent->owner->velocity);
	}
	else	//grapple is inair
	{
		VectorSubtract(ent->s.origin, ent->owner->s.origin, dir);
		len = VectorLength(dir);

		if ((ent->owner->client->grapple_state == 0) || (len > 2000))
		{
			Grapple_Reset(ent);
			return;
		}
	}

	Grapple_DrawCable(ent);
	ent->nextthink = level.time + FRAMETIME;
}

void Grapple_Fire(edict_t* ent)
{
	edict_t* hook;
	vec3_t	offset = { 0 };
	vec3_t	start, f, r;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	//get start point
	AngleVectors(ent->client->v_angle, f, r, NULL);
	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, f, r, start);

	hook = G_Spawn();
	VectorCopy(start, hook->s.origin);
	VectorCopy(f, hook->movedir);
	vectoangles(f, hook->s.angles);
	VectorScale(f, 900, hook->velocity);
	hook->movetype = MOVETYPE_FLYMISSILE;
	hook->clipmask = MASK_SHOT;
	hook->solid = SOLID_BBOX;
	VectorClear(hook->mins);
	VectorClear(hook->maxs);
	hook->s.modelindex = gi.modelindex("models/objects/hook/tris.md2");
	hook->classindex = HOOK; /* MrG{DRGN} */
	hook->owner = ent;
	hook->sounds = 0;
	hook->angle = 0;
	hook->touch = Grapple_Touch;
	hook->think = Grapple_Think;
	hook->nextthink = level.time + FRAMETIME;

	if (ent->client->quad_framenum > level.framenum)
		hook->dmg = 60;
	else
		hook->dmg = 15;

	gi.linkentity(hook);
	ent->client->grapple = hook;
	ent->client->grapple_state = GRAPPLE_INAIR;

	gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/fire.wav"), 1, ATTN_NORM, 0);
}

void Cmd_Hook_f(edict_t* ent)
{
	if (Q_stricmp(gi.argv(1), "grow") == 0)
	{
		if (ent->client->grapple_state > 1 && ent->client->grapple)
		{
			ent->client->grapple->angle += 40;
			if (ent->client->grapple->angle > 2000)
				ent->client->grapple->angle = 2000;

			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/grow.wav"), 1, ATTN_NORM, 0);
		}
	}
	else if (Q_stricmp(gi.argv(1), "shrink") == 0)
	{
		if (ent->client->grapple_state > 1 && ent->client->grapple)
		{
			ent->client->grapple->angle -= 40;
			if (ent->client->grapple->angle < 40)
				ent->client->grapple->angle = 40;

			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/shrink.wav"), 1, ATTN_NORM, 0);
		}
	}
	else // switch on off
	{
		if (ent->client->grapple_state == GRAPPLE_OFF)
		{
			Grapple_Fire(ent);
			return;
		}
		else if (ent->client->grapple_state > 2)	//grow or shrink
		{
			ent->client->grapple_state = GRAPPLE_ATTACHED;
		}
		else
		{
			ent->client->grapple = NULL;
			ent->client->grapple_state = GRAPPLE_OFF;
		}
	}
}

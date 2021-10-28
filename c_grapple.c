#include "g_local.h"
#include "c_grapple.h"

void GrappleInitVars(void)
{
#ifdef	CHAOS_RETAIL
	ban_grapple = gi.cvar("ban_grapple", "1", CVAR_LATCH);
#else
	ban_grapple = gi.cvar("ban_grapple", "0", CVAR_LATCH);	
#endif
	start_grapple = gi.cvar("start_grapple", "0", CVAR_LATCH);
}
void Shutoff_Grapple(edict_t* ent)
{
	ent->client->grapple_state = GRAPPLE_OFF;
	ent->client->grapple = NULL;
}
void Grapple_Reset(edict_t* ent)
{
	if (!ent || !ent->owner)
	{
		return;
	}

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

	if (!ent)
	{
		return;
	}

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

	// MrG{DRGN} 
	if (!ent || !other) /* plane is unused, surf can be NULL */
	{
		G_FreeEdict(ent);
		return;
	}

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

	if (!ent)
	{
		return;
	}

	if (ent->owner->client->grapple_state > GRAPPLE_INAIR)	// Grapple is attached
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

		if ((ent->owner->client->grapple_state == GRAPPLE_OFF) || (len > 2000))
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

	if (!ent)
	{
		return;
	}

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
	hook->classname = "hook"; // MrG{DRGN} 
	hook->classindex = HOOK; // MrG{DRGN} 
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
		if (ent->client->grapple_state > GRAPPLE_INAIR && ent->client->grapple)
		{
			ent->client->grapple->angle += 40;
			if (ent->client->grapple->angle > 2000)
				ent->client->grapple->angle = 2000;

			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/grapple/grow.wav"), 1, ATTN_NORM, 0);
		}
	}
	else if (Q_stricmp(gi.argv(1), "shrink") == 0)
	{
		if (ent->client->grapple_state > GRAPPLE_INAIR && ent->client->grapple)
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
		else if (ent->client->grapple_state > GRAPPLE_ATTACHED)	//grow or shrink
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

void Cmd_Grapple_f(edict_t* ent)
{
	/* MrG{DRGN} if you haven't joined a team yet. you can't use the hook! */
	if (ent->client->resp.spectator)
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->fakedeath > 0)
		return;

	if (ent->client->pers.inventory[ITEM_INDEX(it_grapple)] >= 1)
		Cmd_Hook_f(ent);
	else
	{
		if (!ent->bot_player)// MrG{DRGN} 
			gi.centerprintf(ent, "\nYou don't have a grappling hook!\n");//MATTHIAS

		return;
	}
}
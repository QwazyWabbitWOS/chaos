#include "g_local.h"


#define CAMERA_SWITCH_TIME  15


void CreateCamera(edict_t* ent)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */
	gi.unlinkentity(ent);

	ent->groundentity = NULL;
	ent->takedamage = DAMAGE_NO;
	ent->movetype = MOVETYPE_FLY;
	ent->viewheight = 0;
	ent->s.origin[2] += 24;
	ent->classindex = CAMPLAYER;
	ent->classname = "camera";
	ent->bot_player = 0; /* MrG{DRGN} paranoia */
	ent->mass = 0;
	ent->model = "models/objects/camera/tris.md2";
	ent->solid = SOLID_TRIGGER;
	ent->deadflag = DEAD_NO;
	ent->clipmask = MASK_ALL;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = FL_FLY;
	ent->client->camera = 1;
	ent->client->ps.fov = 90;
	/* MrG{DRGN} no kami stuck screen! */
	ent->client->kamikazetime = 0;

	ent->client->grapple = NULL;
	ent->client->grapple_state = GRAPPLE_OFF;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->BlindTime = 0;
	ent->client->PoisonTime = 0;
	ent->client->invisible = 0;
	ent->client->nextscannercell = 0;
	ent->client->scanneractive = 0;
	/* END */
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->invisible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->jet_framenum = 0;
	ent->client->flashlightactive = 0;
	if (ent->client->flashlight)
	{
		ent->client->flashlight->think = G_FreeEdict;
		G_FreeEdict(ent->client->flashlight);
	}
	if (ent->client->teleporter)
		G_FreeEdict(ent->client->teleporter);

	if (ent->client->b_target)
		G_FreeEdict(ent->client->b_target);

	ent->client->resp.score = 0;
	ent->health = 0;
	ent->client->ps.gunindex = 0;
	ent->client->weapon_sound = 0;
	ent->s.sound = 0;
	ent->s.effects = 0;
	ent->s.skinnum = 0;
	ent->s.modelindex = REMOVED_MODEL;
	ent->s.modelindex2 = REMOVED_MODEL;
	ent->s.modelindex3 = REMOVED_MODEL;
	ent->s.modelindex4 = REMOVED_MODEL;
	ent->s.frame = 0;

	ent->client->showscores = false;
	ent->client->showinventory = false;
	ent->client->showhelp = false;
	ent->client->pers.hand = CENTER_HANDED;
	ent->client->ps.stats[STAT_HEALTH_ICON] = 0;
	ent->client->pers.weapon = NULL;
	ent->client->ps.gunindex = 0;
	ent->client->resp.ctf_team = CTF_NOTEAM;

	/* MrG{DRGN} moved these up here, since I clear the clients inventory below! */
	CTFDeadDropFlag(ent);
	CTFDeadDropTech(ent);
	/* END */
	memset(ent->client->ps.stats, 0, sizeof(ent->client->ps.stats));
	memset(ent->client->pers.inventory, 0, sizeof(ent->client->pers.inventory)); /* MrG{DRGN} to avoid items being used */
	VectorClear(ent->maxs);
	VectorClear(ent->mins);

	gi.linkentity(ent);
	gi.setmodel(ent, ent->model);
}

int NumVisiblePlayers(edict_t* ent)
{
	int		i, num = 0;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return false;
	}
	/* END */

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->health < 0)
			continue;
		if (players[i] == ent)
			continue;
		if (players[i]->client->camera)
			continue;

		if (visible2(players[i]->s.origin, ent->s.origin))
		{
			num++;
		}
	}
	return num;
}

edict_t* ClosestVisible(edict_t* ent)
{
	vec3_t	vdist = { 0 };
	edict_t* best = NULL;
	int		i;
	vec_t	dist, bestdist = 9999;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return NULL;
	}
	/* END */

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->health < 0)
			continue;
		if (players[i] == ent)
			continue;
		if (players[i]->client->camera)
			continue;

		if (visible2(players[i]->s.origin, ent->s.origin))
		{
			VectorSubtract(players[i]->s.origin, ent->s.origin, vdist);
			dist = VectorLength(vdist);

			if (dist < bestdist)
			{
				best = players[i];
				bestdist = dist;
			}
		}
	}

	return best;
}

edict_t* BestViewPlayer(void)
{
	edict_t* best = NULL;
	int	views, bestviews = -1, i;

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->health < 0)
			continue;
		if (players[i]->client->camera)
			continue;

		views = NumVisiblePlayers(players[i]);

		if (views > bestviews)
		{
			bestviews = views;
			best = players[i];
		}
	}
	return best;
}

edict_t* GetFirstValidPlayer(void)
{
	int i;

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;

		return players[i];
	}
	return NULL;
}

edict_t* GetRandomValidPlayer(void)
{
	int i;

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;
		if (random() < 0.5)
			continue;

		return players[i];
	}
	return NULL;
}

int  FirstValidPlayer(void)
{
	int i;

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;

		return i;
	}
	return -1;
}

int LastValidPlayer(void)
{
	int i;

	for (i = (numplayers - 1); i >= 0; i--)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;

		return i;
	}
	return -1;
}

edict_t* GetNextValidPlayer(edict_t* current)
{
	int i;

	/* MrG{DRGN} sanity check*/
	if (!current)
	{
		return NULL;
	}
	/* END */

	//find num of current target
	for (i = 0; i < numplayers; i++)
		if (players[i] == current)
			break;

	if (i == LastValidPlayer())	//last player so switch to first
		i = 0;
	else
		i++;	//start with player after current target

	for (; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;
		if (!players[i]->inuse)
			continue;

		return players[i];
	}

	//found none so stay at current target
	return current;
}

edict_t* GetPrevValidPlayer(edict_t* current)
{
	int i;

	/* MrG{DRGN} sanity check*/
	if (!current)
	{
		return NULL;
	}
	/* END */

	//find num of current target
	for (i = 0; i < numplayers; i++)
		if (players[i] == current)
			break;

	if (i == FirstValidPlayer())	//first player so switch to last
		i = numplayers - 1;
	else
		i--;	//start with player before current target

	for (; i >= 0; i--)
	{
		if (!players[i]->client)
			continue;
		if (players[i]->client->camera)
			continue;
		if (!players[i]->inuse)
			continue;

		return players[i];
	}

	//found none so stay at current target
	return current;
}

void CamNext(edict_t* ent)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	if (ent->client->cammode > 1)
	{
		if ((ent->client->pTarget != NULL) && ent->client->pTarget->client && ent->client->pTarget->inuse)
		{
			ent->client->pTarget = GetNextValidPlayer(ent->client->pTarget);
		}
		else
			ent->client->pTarget = GetFirstValidPlayer();
	}
	else
		cprint_botsafe(ent, PRINT_HIGH, "Target switching only works in cam modes 2, 3 and 4!\n");
}

void CamPrev(edict_t* ent)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	if (ent->client->cammode > 1)
	{
		if ((ent->client->pTarget != NULL) && ent->client->pTarget->client && ent->client->pTarget->inuse)
		{
			ent->client->pTarget = GetPrevValidPlayer(ent->client->pTarget);
		}
		else
			ent->client->pTarget = GetFirstValidPlayer();
	}
	else
		cprint_botsafe(ent, PRINT_HIGH, "Target switching only works in cam modes 2, 3 and 4!\n");
}

void PointCamAtSpot(edict_t* ent, vec3_t spot)
{
	vec3_t	dir = { 0 }, angles;

	/* MrG{DRGN} sanity check*/
	if (!ent || !spot)
	{
		return;
	}
	/* END */

	VectorSubtract(spot, ent->s.origin, dir);

	vectoangles(dir, angles);

	VectorCopy(angles, ent->s.angles);
	VectorCopy(angles, ent->client->ps.viewangles);
	VectorCopy(angles, ent->client->v_angle);
}

void PointCamAtPlayer(edict_t* ent)
{
	vec3_t	dir = { 0 }, angles;
	float	diff;
	int		na;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	VectorSubtract(ent->client->pTarget->s.origin, ent->s.origin, dir);
	vectoangles(dir, angles);

	ent->s.angles[0] = angles[0];
	ent->s.angles[2] = 0;
	diff = angles[1] - ent->s.angles[1];

	na = (int)-angles[0];

	if (na == 0 || na == 180)
	{
		ent->s.frame = 30;
	}
	else if (na > 0 && na <= 90)
	{
		ent->s.frame = (int)na / 3 + 30;
	}
	else if (na > 90 && na < 180)
	{
		ent->s.frame = (int)na / 3;
	}
	else if (na > 180 && na < 270)
	{
		ent->s.frame = (int)(na - 180) / 3;
	}
	else if (na == 270)
	{
		ent->s.frame = 0;
	}
	else if (na > 270 && na <= 360)
	{
		ent->s.frame = (int)(na - 270) / 3;
	}

	while (fabs(diff) > 180.0F)
	{
		if (diff > 0)
		{
			diff -= 360;
		}
		else
		{
			diff += 360;
		}
	}

	if (fabs(diff) > 12)
	{
		if (diff > 0)
		{
			ent->s.angles[1] += 12;
		}
		else
		{
			ent->s.angles[1] -= 12;
		}
	}
	else
	{
		ent->s.angles[1] = angles[1];
	}

	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
	ent->s.angles[0] = 1;
	VectorCopy(ent->s.angles, ent->client->v_angle);
}

void RepositionAtPlayer(edict_t* ent)
{
	vec3_t        diff = { 0 };
	vec3_t        pos = { 0 }, forward;
	trace_t       tr;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	AngleVectors(ent->client->pTarget->client->v_angle, forward, NULL, NULL);
	forward[2] = 0;

	VectorNormalize(forward);

	if (ent->client->cammode == 3)
	{
		pos[0] = ent->client->pTarget->s.origin[0] + forward[0];
		pos[1] = ent->client->pTarget->s.origin[1] + forward[1];
		pos[2] = ent->client->pTarget->s.origin[2] + 100;
	}
	else
	{
		pos[0] = ent->client->pTarget->s.origin[0] + (-100 * forward[0]);
		pos[1] = ent->client->pTarget->s.origin[1] + (-100 * forward[1]);
		pos[2] = ent->client->pTarget->s.origin[2] + 60;
	}

	tr = gi.trace(ent->client->pTarget->s.origin, NULL, NULL, pos,
		ent->client->pTarget, CONTENTS_SOLID);

	if (tr.fraction < 1)
	{
		VectorSubtract(tr.endpos, ent->client->pTarget->s.origin, diff);
		VectorNormalize(diff);
		VectorMA(tr.endpos, -8, diff, tr.endpos);

		if (tr.plane.normal[2] > 0.8)
			tr.endpos[2] += 4;
	}

	if (fabsf(tr.endpos[0] - ent->s.origin[0]) > 12)
	{
		if (tr.endpos[0] > ent->s.origin[0])
		{
			ent->s.origin[0] += 8;
		}
		else
		{
			ent->s.origin[0] -= 8;
		}
	}
	else
	{
		ent->s.origin[0] = tr.endpos[0];
	}

	if (fabsf(tr.endpos[1] - ent->s.origin[1]) > 12)
	{
		if (tr.endpos[1] > ent->s.origin[1])
		{
			ent->s.origin[1] += 8;
		}
		else
		{
			ent->s.origin[1] -= 8;
		}
	}
	else
	{
		ent->s.origin[1] = tr.endpos[1];
	}

	if (fabsf(tr.endpos[2] - ent->s.origin[2]) > 10)
	{
		if (tr.endpos[2] > ent->s.origin[2])
		{
			ent->s.origin[2] += 8;
		}
		else
		{
			ent->s.origin[2] -= 8;
		}
	}
	else
	{
		ent->s.origin[2] = tr.endpos[2];
	}

	tr = gi.trace(ent->client->pTarget->s.origin, NULL, NULL, ent->s.origin,
		ent->client->pTarget, CONTENTS_SOLID);

	if (tr.fraction < 1)
	{
		VectorSubtract(tr.endpos, ent->client->pTarget->s.origin, diff);
		VectorNormalize(diff);
		VectorMA(tr.endpos, -8, diff, tr.endpos);

		if (tr.plane.normal[2] > 0.8)
			tr.endpos[2] += 4;

		VectorCopy(tr.endpos, ent->s.origin);
	}
}

void FindNewTVSpot(edict_t* ent)
{
	edict_t* dummy, * best = NULL;
	vec3_t	dir = { 0 };
	vec_t	dist, bestdist = 9999;

	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */

	dummy = g_edicts;

	for (; dummy < &g_edicts[globals.num_edicts]; dummy++)
	{
		if (!dummy->inuse)
			continue;
		if (dummy == ent)
			continue;
		if (dummy == ent->client->pTarget)
			continue;
		if (dummy->client
			|| dummy->item
			/* MrG{DRGN} classindex instead of classname */
			|| dummy->classindex == BOLT
			|| dummy->classindex == ARROW
			|| dummy->classindex == GRENADE
			|| dummy->classindex == HGRENADE
			|| dummy->classindex == FLASHGRENADE
			|| dummy->classindex == LASERMINE
			|| dummy->classindex == PGRENADE
			|| dummy->classindex == PROXYMINE
			|| dummy->classindex == ROCKET
			|| dummy->classindex == HOMING
			|| dummy->classindex == BUZZ
			|| dummy->classindex == BFG_BLAST
			|| dummy->classindex == ITEM_FLAG_TEAM1
			|| dummy->classindex == ITEM_FLAG_TEAM2
			|| dummy->classindex == BODYQUE
			|| dummy->classindex == SELFTELEPORTER /* MrG{DRGN} added July 9th 2021 */
			|| dummy->classindex == GIBHEAD)
		{
			dummy->s.origin[2] += 40;

			if (!visible(dummy, ent->client->pTarget))
			{
				dummy->s.origin[2] -= 40;
				continue;
			}

			VectorSubtract(dummy->s.origin, ent->client->pTarget->s.origin, dir);
			dummy->s.origin[2] -= 40;

			dist = VectorLength(dir);

			if (dist < 160 && bestdist == 9999) // closer than 160 is not too good, but we take it if we have no other chance
			{
				best = dummy;
				bestdist = dist;
			}
			else if (dist < bestdist || (bestdist < 160))
			{
				best = dummy;
				bestdist = dist;
			}
		}
	}

	if (best)
	{
		vec3_t	angles;

		VectorCopy(best->s.origin, ent->s.origin);
		ent->s.origin[2] += 40;
		VectorSubtract(ent->client->pTarget->s.origin, ent->s.origin, dir);
		vectoangles(dir, angles);

		VectorCopy(angles, ent->s.angles);
		VectorCopy(angles, ent->client->ps.viewangles);
		VectorCopy(angles, ent->client->v_angle);
	}
}

trace_t	PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);

void CameraThink(edict_t* ent, usercmd_t* ucmd)
{
	/* MrG{DRGN} sanity check*/
	if (!ent)
	{
		return;
	}
	/* END */
	//vec3_t	dir = {0};
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.pmove.gravity = 0;
	/* MrG{DRGN} changed to switch cases  form if else */
	switch (ent->client->cammode)
	{
	case 1:	//Intelli Cam mode
	{
		if (NumVisiblePlayers(ent) < 2)
		{
			if (ent->last_move_time >= level.time)
			{
				if (((ent->client->pTarget = BestViewPlayer()) != NULL)
					&& (NumVisiblePlayers(ent->client->pTarget) > 1))
				{
					RepositionAtPlayer(ent);
					PointCamAtPlayer(ent);
				}
				else if ((ent->client->pTarget = ClosestVisible(ent)) != NULL)
				{
					RepositionAtPlayer(ent);
					PointCamAtPlayer(ent);
				}
				else if ((ent->client->pTarget = BestViewPlayer()) != NULL)
				{
					RepositionAtPlayer(ent);
					PointCamAtPlayer(ent);
					ent->last_move_time = 0;
				}
			}
			else if ((ent->client->pTarget = BestViewPlayer()) != NULL)
			{
				RepositionAtPlayer(ent);
				PointCamAtPlayer(ent);
			}
		}
		else if (ent->last_move_time < level.time)
		{
			if (ent->client->pTarget)
			{
				PointCamAtPlayer(ent);
				RepositionAtPlayer(ent);
				ent->last_move_time = level.time + CAMERA_SWITCH_TIME;
			}
		}
		else if (ent->client->pTarget != NULL)
		{
			PointCamAtPlayer(ent);
		}

		if (ent->client->pTarget == NULL)
		{
			ent->client->pTarget = BestViewPlayer();
		}
	}
	break;
	case 2:	//Chase Cam mode
	{
		if ((ent->client->pTarget != NULL) && ent->client->pTarget->client && ent->client->pTarget->inuse)
		{
			RepositionAtPlayer(ent);
			PointCamAtPlayer(ent);
		}
		else
			ent->client->pTarget = GetFirstValidPlayer();
	}
	break;
	case 3:	//Birdview Cam mode
	{
		if ((ent->client->pTarget != NULL) && ent->client->pTarget->client && ent->client->pTarget->inuse)
		{
			RepositionAtPlayer(ent);
			PointCamAtPlayer(ent);
		}
		else
			ent->client->pTarget = GetFirstValidPlayer();
	}
	case 4://TV Cam mode
	{
		if (ent->client->pTarget && ent->client->pTarget->client && ent->client->pTarget->inuse)
		{
			if (visible(ent, ent->client->pTarget))
			{
				PointCamAtPlayer(ent);
			}
			else
			{
				FindNewTVSpot(ent);

				if (visible(ent, ent->client->pTarget))
				{
					PointCamAtPlayer(ent);
				}
				else	// go to next valid player
					ent->client->pTarget = GetRandomValidPlayer();
			}
		}
		else
			ent->client->pTarget = GetFirstValidPlayer();
	}
	}
}

#include "g_local.h"
#include "c_botnav.h"
#include "m_player.h"

qboolean Bot_CanHearClient(edict_t* ent, edict_t* other)
{
	vec3_t	dist = { 0 };

	if (other->mynoise)
	{
		if (!visible(ent, other))
			return false;
	}
	else if (other->mynoise2)
	{
		if (!gi.inPHS(ent->s.origin, other->s.origin))
			return false;
	}
	else
	{
		return false;
	}

	VectorSubtract(other->s.origin, ent->s.origin, dist);

	if (VectorLength(dist) > 1000)	// too far to hear
		return false;

	// check area portals - if they are different and not connected then we can't hear it
	if (other->areanum != ent->areanum)
		if (!gi.AreasConnected(ent->areanum, other->areanum))
			return false;

	return true;
}

edict_t* Bot_FindBestItem(edict_t* ent)
{
	edict_t* best = NULL;
	int		did_weapons = 0, did_health = 0;

	// let's look if we need something very special first:

	// we need health so go for a health pack
	if (ent->health <= 40)
	{
		nprintf(PRINT_HIGH, "%s needs some health!\n", ent->client->pers.netname);

		if ((best = Bot_FindBestHealth(ent)) != 0)
			goto found;

		did_health = 1;
	}

	// we need a better weapon so search one
	if (ent->client->pers.weapon == it_ak42 ||
		ent->client->pers.weapon == it_grenades ||
		ent->client->pers.weapon == it_flashgrenades ||
		ent->client->pers.weapon == it_poisongrenades ||
		ent->client->pers.weapon == it_lasermines)
	{
		nprintf(PRINT_HIGH, "%s needs a better weapon!\n", ent->client->pers.netname);

		if ((best = Bot_FindBestWeapon(ent)) != 0)
			goto found;

		did_weapons = 1;
	}

	// ok we need nothing special, or we haven't found it so search other thingies:

	if (random() > 0.3) // OPTIMIZE: We do not ALWAYS search for POWER-UPS
	{
		// POWER-UPS
		if ((best = Bot_FindBestPowerup(ent)) != 0)
			goto found;
	}

	// WEAPONS
	if (!did_weapons)
	{
		if ((best = Bot_FindBestWeapon(ent)) != 0)
			goto found;
	}

	// HEALTH
	if ((ent->health < (ent->max_health - 20)) && !did_health)
	{
		if ((best = Bot_FindBestHealth(ent)) != 0)
			goto found;
	}

	// AMMO
	best = Bot_FindBestAmmo(ent);

found:

	if (best)
	{
		nprintf(PRINT_HIGH, "%s is going for %s!\n", ent->client->pers.netname, best->classname);
		Bot_CalcPath(ent, best->s.origin, ent->s.origin);
	}

	return best;
}

qboolean Bot_ValidCloseItem(edict_t* ent)
{
	if (!ent->client->b_closeitem)
		return 0;
	if (ent->client->b_goalitem && (ent->client->b_closeitem == ent->client->b_goalitem))
		return 0;
	if (ent->client->b_closeitem->solid != SOLID_TRIGGER)
		return 0;
	if (!visible(ent, ent->client->b_closeitem))
		return 0;
	if (!Bot_CanReachSpotDirectly(ent, ent->client->b_closeitem->s.origin))
		return 0;
	if (!Bot_CanPickupItem(ent, ent->client->b_closeitem))
		return 0;

	return 1;
}

qboolean Bot_StandingUnderPlat(edict_t* ent)
{
	trace_t	tr;
	vec3_t	end = { 0 };

	VectorCopy(ent->s.origin, end);
	end[2] += 1000;

	tr = gi.trace(ent->s.origin, NULL, NULL, end, NULL, MASK_SOLID);

	if (tr.ent && (tr.ent->classindex == FUNC_PLAT))
		return true;
	return false;
}

void Bot_Think(edict_t* ent)
{
	usercmd_t   cmd;
	vec3_t      angles = { 0,0,0 }, mins = { -16,-16,0 }, maxs = { 16,16,10 };

	if (!ent || !ent->client || level.intermissiontime) // MrG{DRGN} stop searching at the end of the map
		return;

	// init usercmd variable
	VectorCopy(ent->client->v_angle, angles);
	VectorSet(ent->client->ps.pmove.delta_angles, 0, 0, 0);
	memset(&cmd, 0, sizeof(usercmd_t));

	//reset duckflag
	ent->client->b_duck = 0;

	// look for enemies
	if (!ent->enemy || random() < 0.1)
		Bot_FindEnemy(ent);

	// look for items close to our way
	if (!Bot_ValidCloseItem(ent))
	{
		if (ent->client->b_closeitem)
			ent->client->b_closeitem->avoidtime = level.time + 10;
		ent->client->b_closeitem = Bot_FindCloseItem(ent);
	}

	// look whether our goalitem is valid or not
	if (ent->client->b_goalitem && (ent->client->b_goalitem->solid != SOLID_TRIGGER))
		ent->client->b_goalitem = NULL;

	// search a goalitem if we do not have an enemy
	if (!ent->client->b_goalitem && (ent->client->b_nextroam <= level.time) && !ent->enemy)
	{
		ent->client->b_nextroam = level.time + 2;

		if (Bot_FindNode(ent, 180, ALL_NODES))
			ent->client->b_goalitem = Bot_FindBestItem(ent);
	}

	// randomly change the strafe direction
	if (level.time >= ent->client->b_strafechange)
	{
		if (random() >= 0.5)
			ent->client->b_strafedir = 0;
		else
			ent->client->b_strafedir = 1;
		ent->client->b_strafechange = level.time + 0.3F + random();
	}

	// randomly change the rundir for no-path-roaming
	if (level.time >= ent->client->b_runchange)
	{
		if (random() >= 0.5)
			ent->client->b_rundir = 0;
		else
			ent->client->b_rundir = 1;
		ent->client->b_runchange = level.time + 3 * random();
	}

	if (ent->client->b_currentnode)
	{
		ent->client->b_currentnode = RecalculateCurrentNode(ent);
	}

	// DEAD
	if (ent->deadflag == DEAD_DEAD)
	{
		if (level.time >= ent->client->b_respawntime)
			cmd.buttons = BUTTON_ATTACK;
	}
	// FLASHLIGHT
	if (lightsoff->value > 0 && ent->client->flashlightactive == 0 && ent->health > 30)
	{
		Cmd_Flashlight_f(ent);
	}
	else if ((lightsoff->value == 0 && ent->client->flashlightactive == true)
		|| (ent->health <= 30 && ent->client->flashlightactive == true))
	{
		ShutOff_Flashlight(ent);
	}
	// PAUSE (WAIT FOR SOMETHING LIKE WAVE ANIMATIONS)
	else if (ent->client->b_pausetime > level.time)
	{
		goto finishmove;
	}
	// ENEMY
	else if (ent->enemy)
	{
		vec3_t		forward, right, dir = { 0 }, oorigin = { 0 }, targ;
		vec_t		dist;
		trace_t		tr;

		if (visible(ent, ent->enemy))
		{
			if ((ent->enemy->health <= 0) || !ent->enemy->inuse)
			{
				ent->enemy = NULL;
			}
			else if (ent->enemy->client
				&& (ent->enemy->client->invisible_framenum > level.framenum)
				&& random() < 0.1)	//invisible
			{
				ent->enemy = NULL;
			}
			else
			{
				// pick best weapon
				if (ent->client->b_nextwchange < level.time)	//not to often
				{
					VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
					dist = VectorLength(dir);
					if (dist > IDEAL_ENEMY_DIST)
						Bot_BestFarWeapon(ent);
					else if (dist >= MELEE_DIST)
						Bot_BestMidWeapon(ent);
					else
						Bot_BestCloseWeapon(ent);
				}

				if (ent->client->b_closeitem != NULL)	//close item
				{
					Bot_Aim(ent, ent->client->b_closeitem->s.origin, angles);
					Bot_Move(ent, &cmd);
				}
				else
				{
					// strafe
					AngleVectors(ent->client->v_angle, forward, right, NULL);
					VectorNormalize(right);

					if (ent->client->b_strafedir == 0) //right
						VectorMA(ent->s.origin, 32, right, dir);
					else
						VectorMA(ent->s.origin, -32, right, dir);

					VectorCopy(ent->s.origin, oorigin);
					oorigin[2] += 24;

					// MrG{DRGN} operator has equivalent nested operands
					// tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_SOLID | MASK_PLAYERSOLID);
					tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_PLAYERSOLID);

					if ((tr.fraction != 1) || tr.startsolid)
					{
						if (ent->client->b_strafedir == 0)
							ent->client->b_strafedir = 1;
						else
							ent->client->b_strafedir = 0;
					}

					if (SaveMoveDir(ent, 0, STRAFE_SPEED, angles))
					{
						if (CheckFall(ent, 0, STRAFE_SPEED, angles)
							|| random() < 0.3)
							Bot_Strafe(ent, &cmd, ent->client->b_strafedir, STRAFE_SPEED, angles);
						else
						{
							if (ent->client->b_strafedir == 0)
								ent->client->b_strafedir = 1;
							else
								ent->client->b_strafedir = 0;
						}
					}
					else
					{
						if (ent->client->b_strafedir == 0)
							ent->client->b_strafedir = 1;
						else
							ent->client->b_strafedir = 0;
					}

					//move to enemy or away
					VectorScale(ent->enemy->velocity, FRAMETIME, targ);
					VectorAdd(ent->enemy->s.origin, targ, targ);
					Bot_Aim(ent, targ, angles);

					VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
					dist = VectorLength(dir);

					if (!ent->client->pers.weapon)
					{
						VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
						dist = VectorLength(dir);
						if (dist > IDEAL_ENEMY_DIST)
							Bot_BestFarWeapon(ent);
						else if (dist >= MELEE_DIST)
							Bot_BestMidWeapon(ent);
						else
							Bot_BestCloseWeapon(ent);
					}

					else if ((ent->client->pers.weapon->classindex == W_SWORD)
						|| (ent->client->pers.weapon->classindex == W_CHAINSAW))
					{
						if (dist >= 50)
						{
							if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
							{
								if (CheckFall(ent, RUN_SPEED, 0, angles)
									|| random() < 0.3)
									cmd.forwardmove = RUN_SPEED;
							}
						}
					}
					else
					{
						if (dist >= IDEAL_ENEMY_DIST)
						{
							if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
							{
								if (CheckFall(ent, RUN_SPEED, 0, angles) || random() < 0.3)
									cmd.forwardmove = RUN_SPEED;
							}
						}
						else
						{
							/* trace_t	tr; MrG{DRGN} redundant declaration! */

							AngleVectors(ent->client->v_angle, forward, NULL, NULL);
							VectorMA(ent->s.origin, -30, forward, forward);

							// MrG{DRGN} operator had equivalent nested operands
							// tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_SOLID | MASK_PLAYERSOLID);
							tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_PLAYERSOLID);

							if ((tr.fraction != 1) || tr.startsolid)
							{
								if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
								{
									cmd.forwardmove = RUN_SPEED;
									cmd.sidemove = crandom() * STRAFE_SPEED;
									cmd.upmove = RUN_SPEED;
								}
							}
							else
							{
								if (SaveMoveDir(ent, -RUN_SPEED, 0, angles))
								{
									if (CheckFall(ent, -RUN_SPEED, 0, angles) || random() < 0.3)
										cmd.forwardmove = -RUN_SPEED;
								}
							}
						}
					}

					if (level.time >= ent->client->b_nextrandjump)
					{
						if (SaveMoveDir(ent, cmd.forwardmove, cmd.sidemove, angles))
						{
							Bot_Jump(ent, &cmd);
						}
						ent->client->b_nextrandjump = level.time + 4 + 6 * random();
					}

					if (visible2(ent->s.origin, targ) && infront(ent, ent->enemy))
						Bot_Attack(ent, &cmd, angles, targ);
				}
			}
		}
		else	//enemy not visible
		{
			//Try to get to the last position we know of the enemy

			if (Bot_CalcPath(ent, ent->enemy->s.origin, ent->s.origin))
			{
				edict_t* mark;

				mark = G_Spawn();
				mark->classname = "enemy_mark";
				mark->classindex = ENEMY_MARK;
				VectorSet(mark->mins, -8, -8, -8);
				VectorSet(mark->maxs, 8, 8, 8);
				mark->owner = ent;
				mark->solid = SOLID_TRIGGER;
				mark->svflags = SVF_NOCLIENT;
				mark->nextthink = level.time + 15;
				mark->think = G_FreeEdict;
				ent->client->b_goalitem = mark;
				nprintf(PRINT_HIGH, "%s's enemy is out of sight but they found a path to the last known position!\n", ent->client->pers.netname);
				gi.linkentity(mark);
			}
			ent->enemy = NULL;
		}
	}

	// ON PLATFORM
	else if (Riding_Plat(ent))
	{
		vec3_t	center, dir = { 0 };

		// find center of plat
		if (ent->groundentity)
		{
			VectorSubtract(ent->groundentity->maxs, ent->groundentity->mins, dir);
			VectorMA(ent->groundentity->mins, 0.5, dir, center);
			center[2] = ent->s.origin[2];

			VectorSubtract(center, ent->s.origin, dir);

			if (VectorLength(dir) > 50)
			{
				Bot_Aim(ent, center, angles);
				Bot_Move(ent, &cmd);
			}
			else
			{
				// look for enemies
				if (ent->client->b_strafedir == 0)
				{
					if (angles[YAW] < 360)
						angles[YAW] += 15;
					else
						angles[YAW] = 0;
				}
				else
				{
					if (angles[YAW] > -360)
						angles[YAW] -= 15;
					else
						angles[YAW] = 0;
				}
			}
		}

		ent->client->b_nodetime = level.time + 3.0F;
	}
	// FOLLOW PATH
	else if (ent->client->b_goalitem)
	{
		int		n;
		vec3_t	dvec = { 0 };
		vec_t	dist;

		// HAVE ROCKET TURRET
		if ((numturrets < 3)
			&& ent->client->pers.inventory[ITEM_INDEX(it_rturret)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_lturret
			&& ent->client->pers.weapon != it_proxyminelauncher)
		{
			if (ent->client->pers.weapon != it_rturret && ent->client->newweapon != it_rturret)
			{
				ent->client->newweapon = it_rturret;
			}
			else
			{
				ent->client->b_nextshot = level.time + 1;
				cmd.buttons = BUTTON_ATTACK;
			}
		}
		// HAVE LASER TURRET
		if ((numturrets < 3)
			&& ent->client->pers.inventory[ITEM_INDEX(it_lturret)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_rturret
			&& ent->client->pers.weapon != it_proxyminelauncher)
		{
			if (ent->client->pers.weapon != it_lturret && ent->client->newweapon != it_lturret)
			{
				ent->client->newweapon = it_lturret;
			}
			else
			{
				ent->client->b_nextshot = level.time + 1;
				cmd.buttons = BUTTON_ATTACK;
			}
		}
		// HAVE PROXIES
		if (ent->client
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxymines)]
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_lturret
			&& ent->client->pers.weapon != it_rturret)
		{
			if (ent->client->pers.weapon != it_proxyminelauncher && ent->client->newweapon != it_proxyminelauncher)
			{
				ent->client->newweapon = it_proxyminelauncher;
			}
			else
			{
				ent->client->b_nextshot = level.time + 0.3F;
				cmd.buttons = BUTTON_ATTACK;
			}
		}

		n = ent->client->b_currentnode;

		//duck if needed
		if (nodes[ent->client->b_path[n]].duckflag)
		{
			ent->client->b_duck = 1;
		}

		if (n < 0)	//go for item
		{
			if (level.time > ent->client->b_nodetime)	//forget that item
			{
				ent->client->b_nodetime = 0;
				ent->client->b_goalitem->avoidtime = level.time + 15;
				ent->client->b_goalitem = NULL;
				ent->client->b_currentnode = -1;
			}
			else if (!visible(ent, ent->client->b_goalitem))
			{
				ent->client->b_nodetime = 0;
				ent->client->b_goalitem->avoidtime = level.time + 15;
				ent->client->b_goalitem = NULL;
				ent->client->b_currentnode = -1;
			}
			else
			{
				dvec[0] = ent->client->b_goalitem->s.origin[0] - ent->s.origin[0];
				dvec[1] = ent->client->b_goalitem->s.origin[1] - ent->s.origin[1];
				dvec[2] = ent->client->b_goalitem->s.origin[2] - ent->s.origin[2];

				dist = VectorLength(dvec);
				if (dist > 30)
				{
					Bot_Aim(ent, ent->client->b_goalitem->s.origin, angles);
					Bot_Move(ent, &cmd);

					//check for jumping out of water
					if (ent->waterlevel)
					{
						if (gi.pointcontents(ent->client->b_goalitem->s.origin) & MASK_WATER)
						{
						}
						else
						{
							nprintf(PRINT_HIGH, "%s tries to jump out of the water!\n", ent->client->pers.netname);
							cmd.upmove = RUN_SPEED;
						}
					}

					if (Node_LavaMove(ent, RUN_SPEED, 0, angles))
						cmd.upmove = RUN_SPEED;

					if (Node_FallMove(ent, RUN_SPEED, 0, angles))
					{
						ent->velocity[0] = 0;
						ent->velocity[1] = 0;
					}

					if (ent->client->b_goalitem->s.origin[2] - ent->s.origin[2] > 20)
						cmd.upmove = ent->client->b_goalitem->s.origin[2] - ent->s.origin[2];
				}
				else
				{
					ent->client->b_nodetime = 0;
					ent->client->b_goalitem = NULL;
					ent->client->b_currentnode = -1;
				}
			}
		}
		else
		{
			//int	lastnodeflag = NORMAL_NODE;

			//if (n > 0)
			//	lastnodeflag = nodes[ent->client->b_path[n - 1]].flag;

			VectorSubtract(nodes[ent->client->b_path[n]].origin, ent->s.origin, dvec);
			dist = VectorLength(dvec);

			if (ent->client->b_closeitem != NULL)
			{
				Bot_Aim(ent, ent->client->b_closeitem->s.origin, angles);

				if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
				{
					//check for jumping out of water
					if (ent->waterlevel)
					{
						if (gi.pointcontents(ent->client->b_closeitem->s.origin) & MASK_WATER)
						{
						}
						else
						{
							nprintf(PRINT_HIGH, "%s tries to jump out of the water!\n", ent->client->pers.netname);
							cmd.upmove = RUN_SPEED;
						}
					}
					Bot_Move(ent, &cmd);
				}
				else
					ent->client->b_closeitem = NULL;
			}
			else if (dist > 30)	//go for node
			{
				if (level.time > ent->client->b_nodetime)	//forget that item
				{
					ent->client->b_nodetime = 0;
					ent->client->b_goalitem->avoidtime = level.time + 10;
					ent->client->b_goalitem = NULL;
					ent->client->b_currentnode = -1;
				}
				else if (!visible_node(nodes[ent->client->b_path[n]].origin, ent->s.origin))
				{
					nprintf(PRINT_HIGH, "visible_node == 0!\n");
					ent->client->b_goalitem->avoidtime = level.time + 10;
					ent->client->b_goalitem = NULL;
					ent->client->b_currentnode = -1;
				}
				else if ((nodes[ent->client->b_path[n]].flag == PLAT_NODE)
					&& dist < 100
					&& visible2(nodes[ent->client->b_path[n]].origin, ent->s.origin))	// next is a plat node
				{
					edict_t* plat = NULL;

					// check if we are standing under the plat
					if (Bot_StandingUnderPlat(ent))
					{
						ent->client->b_nodetime = 0;
						ent->client->b_goalitem->avoidtime = level.time + 10;
						ent->client->b_goalitem = NULL;
						ent->client->b_currentnode = -1;
					}
					else
					{
						//nprintf(PRINT_HIGH,"Next node is a plat node!\n");

						//find the plat
						while ((plat = findradius2(plat, nodes[ent->client->b_path[n]].origin, 300)) != NULL)
						{
							if (plat->classindex == FUNC_PLAT)
							{
								//nprintf(PRINT_HIGH,"Found plat!\n");
								break;
							}
						}

						if (plat && plat->classindex == FUNC_PLAT)
						{
							if (plat->moveinfo.state == STATE_BOTTOM)
							{
								Bot_Aim(ent, nodes[ent->client->b_path[n]].origin, angles);
								Bot_Move(ent, &cmd);
							}
							else
							{
								//nprintf(PRINT_HIGH,"Waiting for plat!\n");

								// look for enemies
								if (ent->client->b_strafedir == 0)
								{
									if (angles[YAW] < 360)
										angles[YAW] += 20;
									else
										angles[YAW] = 0;
								}
								else
								{
									if (angles[YAW] > -360)
										angles[YAW] -= 20;
									else
										angles[YAW] = 0;
								}
							}
						}
						else
						{
							//nprintf(PRINT_HIGH,"Waiting for plat!\n");

							// look for enemies
							if (ent->client->b_strafedir == 0)
							{
								if (angles[YAW] < 360)
									angles[YAW] += 20;
								else
									angles[YAW] = 0;
							}
							else
							{
								if (angles[YAW] > -360)
									angles[YAW] -= 20;
								else
									angles[YAW] = 0;
							}
						}
					}
				}
				else if ((nodes[ent->client->b_path[n]].flag == LADDER_NODE)
					&& visible2(nodes[ent->client->b_path[n]].origin, ent->s.origin))	// next is a ladder node
				{
					if (TouchingLadder(ent))
					{
						cmd.upmove = RUN_SPEED;
					}
					else
					{
						Bot_Aim(ent, nodes[ent->client->b_path[n]].origin, angles);
						Bot_Move(ent, &cmd);
					}
				}
				else	// normal node
				{
					Bot_Aim(ent, nodes[ent->client->b_path[n]].origin, angles);

					if ((n > 0) && nodes[ent->client->b_path[n]].flag != INAIR_NODE
						&& nodes[ent->client->b_path[n - 1]].flag != INAIR_NODE)
					{
						Bot_Move(ent, &cmd);

						//check for jumping out of water
						if (ent->waterlevel)
						{
							if (gi.pointcontents(nodes[ent->client->b_path[n]].origin) & MASK_WATER)
							{
							}
							else
							{
								nprintf(PRINT_HIGH, "%s tries to jump out of the water!\n", ent->client->pers.netname);
								cmd.upmove = RUN_SPEED;
							}
						}

						if (level.time >= ent->client->b_nextrandjump)
						{
							cmd.upmove = RUN_SPEED;
							ent->client->b_nextrandjump = level.time + 4 + 6 * random();
						}

						if (Node_LavaMove(ent, RUN_SPEED, 0, angles))
							cmd.upmove = RUN_SPEED;

						if (Node_FallMove(ent, RUN_SPEED, 0, angles))
						{
							ent->velocity[0] = 0;
							ent->velocity[1] = 0;
						}
					}
					else
					{
						Bot_Move(ent, &cmd);

						if (level.time >= ent->client->b_nextrandjump)
						{
							cmd.upmove = RUN_SPEED;
							ent->client->b_nextrandjump = level.time + 4 + 6 * random();
						}

						if (Node_LavaMove(ent, RUN_SPEED, 0, angles))
							cmd.upmove = RUN_SPEED;
					}

					if ((nodes[ent->client->b_path[n]].origin[2] - ent->s.origin[2] > 20)
						&& !Bot_CanReachSpotDirectly(ent, nodes[ent->client->b_path[n]].origin))
						cmd.upmove = RUN_SPEED;
				}
			}
			else
			{
				//bprint_botsafe(PRINT_HIGH,"currentnode %d!\n",ent->client->b_currentnode);
				ent->client->b_nodetime = level.time + 3.0F;

				if (ent->client->b_currentnode > 0)
				{
					ent->client->b_currentnode = n - 1;
				}
				else	// reached last node
				{
					ent->client->b_nodetime = level.time + 3.0F;
					ent->client->b_currentnode = -1;
				}
			}
		}
	}
	//NO-PATH ROAMING
	else
	{
		vec3_t	forward, dir = { 0 }, oorigin = { 0 }, wallangles;
		trace_t	tr;

		// HAVE ROCKET TURRET
		if ((numturrets < 3)
			&& ent->client->pers.inventory[ITEM_INDEX(it_rturret)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_lturret
			&& ent->client->pers.weapon != it_proxyminelauncher)
		{
			if (ent->client->pers.weapon != it_rturret && ent->client->newweapon != it_rturret)
			{
				ent->client->newweapon = it_rturret;
			}
			else
			{
				ent->client->b_nextshot = level.time + 1;
				cmd.buttons = BUTTON_ATTACK;
			}
		}
		// HAVE LASER TURRET
		if ((numturrets < 3)
			&& ent->client->pers.inventory[ITEM_INDEX(it_lturret)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_rturret
			&& ent->client->pers.weapon != it_proxyminelauncher)
		{
			if (ent->client->pers.weapon != it_lturret && ent->client->newweapon != it_lturret)
			{
				ent->client->newweapon = it_lturret;
			}
			else
			{
				ent->client->b_nextshot = level.time + 1;
				cmd.buttons = BUTTON_ATTACK;
			}
		}
		// HAVE PROXIES
		if (ent->client
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxymines)]
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)]
			&& (ent->client->b_nextshot < level.time)
			&& ent->client->pers.weapon != it_lturret
			&& ent->client->pers.weapon != it_rturret)
		{
			if (ent->client->pers.weapon != it_proxyminelauncher && ent->client->newweapon != it_proxyminelauncher)
			{
				ent->client->newweapon = it_proxyminelauncher;
			}
			else
			{
				ent->client->b_nextshot = level.time + 0.3F;
				cmd.buttons = BUTTON_ATTACK;
			}
		}

		// ITEM
		if (ent->client->b_nopathitem && visible(ent, ent->client->b_nopathitem))
		{
			if (gi.pointcontents(ent->client->b_nopathitem->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME)
				|| ent->client->b_nopathitem->solid != SOLID_TRIGGER)
				ent->client->b_nopathitem = NULL;
			else
			{
				ent->client->b_waittime = 0;
				VectorSubtract(ent->client->b_nopathitem->s.origin, ent->s.origin, dir);

				Bot_Aim(ent, ent->client->b_nopathitem->s.origin, angles);

				if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
				{
					if (CheckFall(ent, RUN_SPEED, 0, angles)
						|| random() < 0.3)
						Bot_Move(ent, &cmd);
					else
					{
						if (ent->client->b_rundir == 0)
							angles[YAW] += 90 + crandom() * 20;
						else
							angles[YAW] -= 90 + crandom() * 20;
					}
					Bot_Move(ent, &cmd);
				}
				else
				{
					ent->client->b_nopathitem = NULL;
					angles[YAW] += 180;
				}

				if (level.time >= ent->client->b_nextrandjump)
				{
					if (SaveMoveDir(ent, cmd.forwardmove, cmd.sidemove, angles))
					{
						Bot_Jump(ent, &cmd);
					}
					ent->client->b_nextrandjump = level.time + 4 + 6 * random();
				}
			}
		}
		else
		{
			ent->client->b_waittime = 0;

			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			VectorNormalize(forward);
			VectorMA(ent->s.origin, 48, forward, dir);

			VectorCopy(ent->s.origin, oorigin);
			oorigin[2] += 24;

			// MrG{DRGN} operator had equivalent nested operands
			// tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_SOLID | MASK_PLAYERSOLID);
			tr = gi.trace(oorigin, mins, maxs, dir, ent, MASK_PLAYERSOLID);

			if ((tr.fraction != 1) || tr.startsolid)
			{
				if (Q_stricmp(tr.ent->classname, "func_door") == 0
					|| Q_stricmp(tr.ent->classname, "func_door_secret") == 0)
				{
					ent->client->b_waittime = level.time + 2;
				}
				else if (tr.contents)
				{
					vectoangles(tr.plane.normal, wallangles);
					if (ent->client->b_rundir == 0)
						angles[YAW] = wallangles[YAW] + 90 + crandom() * 20;
					else
						angles[YAW] = wallangles[YAW] - 90 + crandom() * 20;
				}
				else
					angles[YAW] += crandom() * 180;
			}

			if (SaveMoveDir(ent, RUN_SPEED, 0, angles))
			{
				if (CheckFall(ent, RUN_SPEED, 0, angles)
					|| random() < 0.2)
					Bot_Move(ent, &cmd);
				else
				{
					if (ent->client->b_rundir == 0)
						angles[YAW] += 90 + crandom() * 20;
					else
						angles[YAW] -= 90 + crandom() * 20;
				}
			}
			else
			{
				angles[YAW] += crandom() * 180;
			}

			ent->client->b_nopathitem = Bot_FindItem(ent);
		}
	}

finishmove:

	cmd.msec = 100;

	cmd.angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
	cmd.angles[YAW] = ANGLE2SHORT(angles[YAW]);
	cmd.angles[ROLL] = ANGLE2SHORT(angles[ROLL]);

	ClientThink(ent, &cmd);

	ent->nextthink = level.time + FRAMETIME;
}

int Riding_Plat(edict_t* ent)
{
	vec3_t	dest = { 0 };
	trace_t	tr;

	VectorCopy(ent->s.origin, dest);
	dest[2] -= 50;

	tr = gi.trace(ent->s.origin, NULL, NULL, dest, ent, MASK_SOLID);

	if (tr.ent && strcmp(tr.ent->classname, "func_plat") == 0 && tr.ent->moveinfo.state == STATE_UP)
		return 1;

	return 0;
}

edict_t* Bot_FindBestWeapon(edict_t* ent)
{
	edict_t* current = weapon_list;
	edict_t* best = NULL;
	int		dist, bonus = 0, best_dist = 999;

	// go through all weapons on the level
	// find the closest weapon
	while (current)
	{
		if (current->avoidtime > level.time)
			goto next;

		if (current->solid != SOLID_TRIGGER)	// is it currently there
			goto next;

		if (!current->item)
			goto next;

		if (!Bot_FindPath(ent, current->s.origin, ent->s.origin))
			goto next;

		dist = first_pathnode;	// length of path to item

		// add weapon bonuses to the dist
		if (current->item == it_rturret
			|| current->item == it_lturret
			|| current->item == it_vortex
			|| current->item == it_bfg)
		{
			// We really, really want these !!!
			bonus = 6;

			if ((current->item->classindex == AM_RTURRET && ent->client->pers.inventory[ITEM_INDEX(it_rturret)] >= ent->client->pers.max_rturret)
				|| (current->item->classindex == AM_LTURRET && ent->client->pers.inventory[ITEM_INDEX(it_lturret)] >= ent->client->pers.max_lturret)
				|| (current->item->classindex == AM_VORTEX && ent->client->pers.inventory[ITEM_INDEX(it_vortex)] >= ent->client->pers.max_vortex))
			{
				current = current->next_listitem;
				continue;
			}
		}
		else if (current->item == it_buzzsaw
			|| current->item == it_railgun
			|| current->item == it_rocketlauncher
			|| current->item == it_proxyminelauncher
			|| current->item == it_sword)
		{
			// We really want these !!
			bonus = 4;
		}
		else if (current->item == it_hyperblaster
			|| current->item == it_supershotgun
			|| current->item == it_grenadelauncher)
		{
			// Not to bad !
			bonus = 2;
		}

		else if (current->item == it_chainsaw
			|| current->item == it_airfist)
		{
			// Still better than the rest !
			bonus = 0;
		}
		else
		{
			// Not so good !
			bonus = -2;
		}

		dist = dist - bonus;

		if (dist < best_dist)
		{
			best_dist = dist;
			best = current;
		}

	next:
		// try the next weapon
		current = current->next_listitem;
	}

	return best;
}

edict_t* Bot_FindBestHealth(edict_t* ent)
{
	edict_t* current = health_list;
	edict_t* best = NULL;
	int		dist, bonus = 0, best_dist = 999;

	// go through all health packs on the level
	while (current)
	{
		if (current->avoidtime > level.time)
			goto next;

		if (current->solid != SOLID_TRIGGER)	// is it currently there
			goto next;

		if (!current->item)
			goto next;

		if (!Bot_FindPath(ent, current->s.origin, ent->s.origin))	//is there a path to that item
			goto next;

		dist = first_pathnode;	// length of path to item

		// add bonuses to the dist
		if (current->item == it_health_mega)
		{
			// We really, really want these !!!
			bonus = 6;
		}
		else if (current->item == it_health_large && Bot_NeedsHealth(ent, current))/* MrG{DRGN} was missing current->item == */
		{
			// We really want these !!
			bonus = 4;
		}
		else if (current->item == it_health && Bot_NeedsHealth(ent, current))/* MrG{DRGN} was missing current->item == */
		{
			// Not to bad !
			bonus = 0;
		}
		if (current->item == it_health_small)
		{
			// We'll take it!
			bonus = 2;
		}

		dist = dist - bonus;

		if (dist < best_dist)
		{
			best_dist = dist;
			best = current;
		}

	next:
		// try the next health pack
		current = current->next_listitem;
	}
	return best;
}

edict_t* Bot_FindBestPowerup(edict_t* ent)
{
	edict_t* current = powerup_list;
	edict_t* best = NULL;
	int		dist, bonus = 0, best_dist = 999;

	// go through all powerups on the level
	while (current)
	{
		if (current->avoidtime > level.time)
			goto next;

		if (current->solid == SOLID_TRIGGER)	// is it currently there
			goto next;

		if (!current->item)
			goto next;

		// don't go for stuff the bot can't use yet or that just isn't worth it
		if (current->item != it_quaddamage
			&& current->item != it_invulnerability
			&& current->item != it_tech1
			&& current->item != it_tech2
			&& current->item != it_tech3
			&& current->item != it_tech4
			&& current->item != it_invisibility
			&& current->item != FindItemByClassindex(AR_POWER_SHIELD)
			&& current->item != FindItemByClassindex(AR_POWER_SCREEN)
			&& current->item != FindItemByClassindex(AR_BODY)
			&& current->item != FindItemByClassindex(AR_JACKET)
			&& current->item != FindItemByClassindex(AR_COMBAT)
			&& current->item != it_adrenaline)
			goto next;

		if ((ent->health < (ent->max_health - 20)) && it_adrenaline)
			goto next;

		// check if it's a tech and if we already have one
		if (current->item == it_tech2
			|| current->item == it_tech3
			|| current->item == it_tech4
			|| current->item == it_tech1)
		{
			if (ent->client->pers.inventory[ITEM_INDEX(it_tech2)]
				|| ent->client->pers.inventory[ITEM_INDEX(it_tech3)]
				|| ent->client->pers.inventory[ITEM_INDEX(it_tech4)]
				|| ent->client->pers.inventory[ITEM_INDEX(it_tech1)])
				goto next;
		}

		if (!Bot_FindPath(ent, current->s.origin, ent->s.origin))
			goto next;

		dist = first_pathnode;	// length of path to item

		// add bonuses to the dist
		if ((current->item == it_quaddamage)
			|| current->item == it_invulnerability
			|| current->item == it_invisibility
			|| current->item == it_tech1
			|| current->item == it_tech2
			|| current->item == it_tech3
			|| current->item == it_tech4)
		{
			// We really, really want these !!!
			bonus = 6;
		}
		else if (current->item == FindItemByClassindex(AR_POWER_SHIELD)
			|| current->item == FindItemByClassindex(AR_POWER_SCREEN)
			|| current->item == FindItemByClassindex(AR_BODY)
			|| current->item == FindItemByClassindex(AR_JACKET)
			|| current->item == FindItemByClassindex(AR_COMBAT))
		{
			// We really want these !!
			bonus = 4;
		}
		else if (current->item == it_adrenaline
			|| current->item == it_bandolier
			|| current->item == it_pack
			|| current->item == it_jetpack)
		{
			// Not to bad !
			bonus = 2;
		}
		else if (current->item == it_grapple
			|| current->item == it_silencer
			|| current->item == it_breather
			|| current->item == it_enviro)
		{
			// ok !
			bonus = 0;
		}
		else
			bonus = -999; //don't go for other stuff

		dist = dist - bonus;

		if (dist < best_dist)
		{
			best_dist = dist;
			best = current;
		}

	next:
		// try the next powerup
		current = current->next_listitem;
	}
	return best;
}

edict_t* Bot_FindBestAmmo(edict_t* ent)
{
	edict_t* current = ammo_list;
	edict_t* best = NULL;
	int		dist, best_dist = 999;

	// go through all ammo packs on the level
	while (current)
	{
		if (current->avoidtime > level.time)
			goto next;

		if (current->solid == SOLID_TRIGGER)	// is it currently there
			goto next;

		if (!current->item)
			goto next;

		if (current->item->classindex == AM_SHELLS	// some ammo isn't worth calculating a path it can be picked up as a closeitem
			|| current->item->classindex == AM_EXPLOSIVESHELLS
			|| current->item->classindex == AM_LASERGRENADES)
			goto next;

		if (!Bot_CanPickupAmmo(ent, current))
			goto next;

		if (!Bot_FindPath(ent, current->s.origin, ent->s.origin))
			goto next;

		dist = first_pathnode;	// length of path to item

		if (dist < best_dist)
		{
			best_dist = dist;
			best = current;
		}

	next:
		// try the next ammo pack
		current = current->next_listitem;
	}
	return best;
}

int Bot_CalcPath(edict_t* ent, vec3_t targ, vec3_t source)
{
	int sn, tn, i;

	// can't reach our current targ
	if ((tn = Bot_FindNodeAtEnt(targ)) < 0)
	{
		ent->client->b_currentnode = -1;
		return 0;
	}

	if ((sn = Bot_FindNodeAtEnt(source)) >= 0)
	{
		if (Bot_ShortestPath(sn, tn))
		{
			ent->client->b_currentnode = first_pathnode;
			ent->client->b_nodetime = level.time + 2;

			for (i = 0; i < 100; i++)
				ent->client->b_path[i] = path_buffer[i];

			//nprintf(PRINT_HIGH,"Found path from %d to %d! %d is the first node!\n", sn, tn, path_buffer[first_pathnode]);
		}
		else // can't reach our current targ
		{
			ent->client->b_currentnode = -1;
			return 0;
		}
	}
	return 1;
}

int Bot_FindPath(edict_t* ent, vec3_t targ, vec3_t source)
{
	int sn, tn;

	// can't reach our current targ
	if ((tn = Bot_FindNodeAtEnt(targ)) < 0)
		return 0;

	if ((sn = Bot_FindNodeAtEnt(source)) < 0)
		return 0;

	if (Bot_ShortestPath(sn, tn))
		return 1;
	return 0;
}

void Bot_Aim(edict_t* ent, const vec3_t targ, vec3_t angles)
{
	vec3_t dir = { 0 }, ideala;
	float	ideal;
	float	current;
	float	move;
	float	speed;

	VectorSubtract(targ, ent->s.origin, dir);

	if (!ent->waterlevel)
	{
		if (dir[2] < 0)
			dir[2] = 0;
	}

	vectoangles(dir, ideala);

	/* MrG{DRGN} now (20 degrees per frame base, + 5 degrees per level)
	speed = 90;
	*/
	speed = ent->yaw_speed;

	for (int i = 0; i < 2; i++)
	{
		current = anglemod(ent->client->v_angle[i]);
		ideal = ideala[i];

		if (current == ideal)
			continue;

		move = ideal - current;
		speed = ent->yaw_speed;

		if (ideal > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}

		angles[i] = anglemod(current + move);

		if (ent->client->BlindTime > 0)
		{
			angles[YAW] += (random() * 16 - 8) * ent->client->BlindTime;
		}
		if (ent->client->PoisonTime > 0)
		{
			angles[YAW] += (random() * 16 - 8) * ent->client->PoisonTime;
		}
	}

	/*
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	*/
}

void Bot_Attack(edict_t* ent, usercmd_t* cmd, vec3_t angles, vec3_t targ)
{
	vec3_t	dir = { 0 }, t_angles;
	gitem_t* weapon;
	int temp_level = (6 - ent->client->b_botlevel); // MrG{DRGN} Arithmetic overflow: shut up code analysis

	if (ent->enemy->deadflag == DEAD_DEAD)
		ent->enemy = NULL;

	if (ent->enemy)
	{
		// fire
		if (level.time >= ent->client->b_nextshot)
		{
			weapon = ent->client->pers.weapon;
			if (!weapon)
				return;
			/* MrG{DRGN} ty Paril! */
			// if the enemy is above us, aim for their head
			if (targ[2] - ent->s.origin[2] > 8)
				targ[2] += ent->enemy->viewheight;
			// aim explosives at feet

			else if ((weapon->classindex == W_ROCKETLAUNCHER) || (weapon->classindex == W_HOMINGLAUNCHER) || (ent->enemy->client && (ent->enemy->client->ps.pmove.pm_flags & PMF_DUCKED)))
				targ[2] -= 12;

			VectorSubtract(targ, ent->s.origin, dir);
			vectoangles(dir, t_angles);

			angles[0] = t_angles[0];

			if (weapon->classindex == W_HYPERBLASTER)
			{
				angles[YAW] += crandom() * (ent->client->b_botlevel) / 2;
				angles[PITCH] += crandom() * (ent->client->b_botlevel) / 2;
			}
			else if (weapon->classindex == W_ROCKETLAUNCHER
				|| weapon->classindex == W_HOMINGLAUNCHER)
			{
				angles[YAW] += crandom() * (temp_level) * 0.2F;
				angles[PITCH] += crandom() * (temp_level) * 0.2F;
			}
			else
			{
				angles[YAW] += crandom() * (temp_level) * 0.2F;
				angles[PITCH] += crandom() * (temp_level) * 0.2F;
			}

			cmd->buttons = BUTTON_ATTACK;
			if (CTFApplyHaste(ent))
			{
				float	addtime = Bot_Fire_Freq(ent) / 2;

				if (addtime < 0)
					addtime = 0;

				ent->client->b_nextshot = level.time + addtime;
			}
			else
				ent->client->b_nextshot = level.time + Bot_Fire_Freq(ent);
		}
	}
}

void bot_pain(edict_t* ent, edict_t* other, float kickback, int damage)
{
	if (!ent)
		return;

	if ((ent != other))
	{
		if (other->client && (other->client->invisible_framenum > level.framenum) && random() < 0.3)	//invisible
		{
			ent->enemy = other;
		}
		else if (other->client && (other->client->invisible_framenum <= level.framenum))
		{
			ent->enemy = other;
		}
	}

	if (ent->client)
		ent->client->b_pausetime = 0;

	player_pain(ent, other, kickback, damage);
}

void Bot_Move(edict_t* ent, usercmd_t* cmd)
{
	cmd->forwardmove = RUN_SPEED;
}

void Bot_Strafe(edict_t* ent, usercmd_t* cmd, int strafedir, short speed, vec3_t angles)
{
	if (strafedir == 0)
	{
		if (SaveMoveDir(ent, 0, speed, angles))
		{
			cmd->sidemove = speed;
		}
		else if (SaveMoveDir(ent, 0, (short)-speed, angles))
		{
			cmd->sidemove = -speed;
			ent->client->b_strafedir = 1;
		}
		else
			cmd->sidemove = 0;
	}

	if (strafedir == 1)
	{
		if (SaveMoveDir(ent, 0, (short)-speed, angles))
		{
			cmd->sidemove = -speed;
		}
		else if (SaveMoveDir(ent, 0, speed, angles))
		{
			cmd->sidemove = speed;
			ent->client->b_strafedir = 0;
		}
		else
			cmd->sidemove = 0;
	}
}

void Bot_Jump(edict_t* ent, usercmd_t* cmd)
{
	cmd->upmove = RUN_SPEED;
}

void Bot_FindEnemy(edict_t* self)
{
	edict_t* newenemy = NULL;
	int			i;
	float		bestdist = 9999;
	vec3_t		dir = { 0 };
	vec_t		dist;

	for (i = 0; i < numturrets; i++)
	{
		if (!turrets[i]->inuse)
			continue;
		if (turrets[i]->owner == self)
			continue;
		if (!visible(self, turrets[i]))
			continue;
		if (turrets[i]->health <= 0)
			continue;
		if (TeamMembers(self, turrets[i]->owner))
			continue;

		VectorSubtract(turrets[i]->s.origin, self->s.origin, dir);
		dist = VectorLength(dir);

		if (dist < bestdist)
		{
			newenemy = turrets[i];
			bestdist = dist;
		}
	}

	if (newenemy && (random() < 0.8))	//go for turrets first
	{
		self->enemy = newenemy;
		return;
	}

	for (i = 0; i < numplayers; i++)
	{
		if (!players[i]->client)
			continue;
		if (players[i] == self)
			continue;
		if (players[i]->client->invincible_framenum > level.framenum)	//invincible
			continue;
		if (!visible(self, players[i]))
			continue;
		if (players[i]->health <= 0)
			continue;
		if (players[i]->client->resp.spectator)
			continue;
		if ((players[i]->client->invisible_framenum > level.framenum) && random() > 0.01)	//invisible
			continue;

		if (TeamMembers(self, players[i]))
			continue;

		VectorSubtract(players[i]->s.origin, self->s.origin, dir);
		dist = VectorLength(dir);

		if (dist >= 200)	//close enemies don't need to be infront
		{
			if (!Bot_CanHearClient(self, players[i]))	//if we hear the enemy he does not have to be infront
			{
				if (lightsoff->value == 1)
				{
					if (!infront2(self, players[i]))
						continue;
				}
				else if (lightsoff->value > 1)
				{
					if (!infront3(self, players[i]))
						continue;
				}
				else
				{
					if (!infront(self, players[i]))
						continue;
				}
			}
			else
				nprintf(PRINT_HIGH, "%s heard %s!\n", self->client->pers.netname, players[i]->client->pers.netname);
		}

		if (dist < bestdist)
		{
			bestdist = dist;
			newenemy = players[i];
		}
	}

	self->enemy = newenemy;
}

edict_t* Bot_FindCloseItem(edict_t* ent)
{
	edict_t* newitem = NULL, * best = NULL;
	vec3_t		dir = { 0 };
	vec_t		dist, bestdist = 99999;

	while ((newitem = findradius(newitem, ent->s.origin, 90)) != NULL)
	{
		if (!newitem->item)
			continue;
		if (ent->client->b_goalitem && (newitem == ent->client->b_goalitem))
			continue;
		if (!visible(ent, newitem))
			continue;
		if (!Bot_CanPickupItem(ent, newitem))
			continue;
		if (gi.pointcontents(newitem->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
			continue;
		if (newitem->avoidtime > level.time)
			continue;
		if (!Bot_NeedsHealth(ent, newitem)) // MrG{DRGN} if we don't need it, don't go for health!
			continue;
		//if (!Bot_CanReachSpotDirectly(ent, newitem->s.origin))
		//	continue;

		VectorSubtract(ent->s.origin, newitem->s.origin, dir);
		dist = VectorLength(dir);

		if (dist < bestdist)
		{
			bestdist = dist;
			best = newitem;
		}
	}

	if (best)
		nprintf(PRINT_HIGH, "%s is going for the %s close to their way.\n", ent->client->pers.netname, best->classname);

	return best;
}

edict_t* Bot_FindItem(edict_t* ent)
{
	edict_t* newitem = NULL, * best = NULL;
	vec3_t		dir = { 0 };
	vec_t		dist, bestdist = 99999;

	while ((newitem = findradius(newitem, ent->s.origin, 500)) != NULL)
	{
		if (!newitem->item)
			continue;

		if (!visible(ent, newitem))
			continue;
		if (random() < 0.1)
			continue;
		if (!Bot_CanPickupItem(ent, newitem))
			continue;
		if (gi.pointcontents(newitem->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
			continue;
		if (newitem->avoidtime > level.time)
			continue;
		if (!Bot_NeedsHealth(ent, newitem)) // MrG{DRGN} if we don't need it, don't go for health!
			continue;
		VectorSubtract(ent->s.origin, newitem->s.origin, dir);
		dist = VectorLength(dir);

		if (dist < bestdist)
		{
			bestdist = dist;
			best = newitem;
		}
	}

	return best;
}

void Bot_FindActivator(edict_t* ent)
{
	edict_t* activator = NULL;

	while ((activator = findradius(activator, ent->s.origin, 300)) != NULL)
	{
		if (activator->item)
			continue;
		if (activator->client)
			continue;
		if (gi.pointcontents(activator->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
			continue;
		if (random() < 0.1)
			continue;
		if (activator->avoidtime > level.time)
			continue;

		if (visible(ent, activator) &&
			(activator->classindex == MISC_TELEPORTER ||
				activator->classindex == FUNC_BUTTON ||
				activator->classindex == FUNC_DOOR))
		{
			ent->client->b_activator = activator;
			break;
		}
	}
}

void Bot_ProjectileAvoidance(edict_t* self, usercmd_t* cmd, vec3_t angles)
{
	edict_t* blip = NULL;

	while ((blip = findradius(blip, self->s.origin, 150)) != NULL)
	{
		if (blip->classindex == ARROW
			|| blip->classindex == PARROW
			|| blip->classindex == EXARROW
			|| blip->classindex == FLASHGRENADE
			|| blip->classindex == LASERMINE
			|| blip->classindex == PGRENADE
			|| blip->classindex == PROXYMINE
			|| blip->classindex == BFG_BLAST)
		{
			if (!visible(self, blip))
				continue;
			if (!infront(self, blip))
				continue;

			Bot_Strafe(self, cmd, self->client->b_strafedir, STRAFE_SPEED, angles);
			self->client->b_strafechange = level.time + 0.5F;
			break;
		}
		if (blip->classindex == ROCKET
			|| blip->classindex == HOMING
			|| blip->classindex == BUZZ
			|| blip->classindex == RT_ROCKET
			|| blip->classindex == GRENADE
			|| blip->classindex == HGRENADE)
		{
			if (!visible(self, blip))
				continue;
			if (!infront(self, blip))
				continue;

			Bot_Strafe(self, cmd, self->client->b_strafedir, STRAFE_SPEED, angles);
			self->client->b_strafechange = level.time + 0.5F;
			Bot_Jump(self, cmd);
			break;
		}
	}
}

qboolean SaveMoveDir(edict_t* self, short forwardmove, short sidemove, vec3_t angles)
{
	vec3_t	new_origin = { 0 };	//origin after move
	vec3_t	forward, right, up;
	vec3_t	trace_end = { 0 };
	vec3_t  mins = { -3, -3, -3 }, maxs = { 3, 3, 3 };
	trace_t	tr;

	if (forwardmove == 0 && sidemove == 0)
		return true;

	AngleVectors(angles, forward, right, up);
	VectorCopy(self->s.origin, new_origin);

	if (forwardmove)
	{
		VectorScale(forward, forwardmove * FRAMETIME, forward);
		VectorAdd(new_origin, forward, new_origin);
	}
	if (sidemove)
	{
		VectorScale(right, sidemove * FRAMETIME, right);
		VectorAdd(new_origin, right, new_origin);
	}

	VectorCopy(new_origin, trace_end);
	trace_end[2] -= 600;

	tr = gi.trace(new_origin, mins, maxs, trace_end, self, MASK_SOLID | MASK_WATER);

	if ((tr.fraction == 1)
		|| (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
		|| (tr.plane.normal[2] < 0.3))
	{
		self->velocity[0] = 0;
		self->velocity[1] = 0;
		return false;
	}
	return true;
}

qboolean Node_FallMove(edict_t* self, short forwardmove, short sidemove, vec3_t angles)
{
	vec3_t	new_origin = { 0 };	//origin after move
	vec3_t	forward, right, up;
	vec3_t	trace_end = { 0 };
	vec3_t  mins = { -3, -3, -3 }, maxs = { 3, 3, 3 };
	trace_t	tr;

	if (forwardmove == 0 && sidemove == 0)
		return false;

	AngleVectors(angles, forward, right, up);
	VectorCopy(self->s.origin, new_origin);

	if (forwardmove)
	{
		VectorScale(forward, forwardmove * FRAMETIME, forward);
		VectorAdd(new_origin, forward, new_origin);
	}
	if (sidemove)
	{
		VectorScale(right, sidemove * FRAMETIME, right);
		VectorAdd(new_origin, right, new_origin);
	}

	VectorCopy(new_origin, trace_end);
	trace_end[2] -= 600;

	tr = gi.trace(new_origin, mins, maxs, trace_end, self, MASK_SOLID | MASK_WATER);

	if ((tr.fraction == 1) || (tr.plane.normal[2] < 0.3))
	{
		return true;
	}
	return false;
}

qboolean Node_LavaMove(edict_t* self, short forwardmove, short sidemove, vec3_t angles)
{
	vec3_t	new_origin = { 0 };	//origin after move
	vec3_t	forward, right, up;
	vec3_t	trace_end = { 0 };
	vec3_t  mins = { -3, -3, -3 }, maxs = { 3, 3, 3 };
	trace_t	tr;

	if (forwardmove == 0 && sidemove == 0)
		return false;

	AngleVectors(angles, forward, right, up);
	VectorCopy(self->s.origin, new_origin);

	if (forwardmove)
	{
		VectorScale(forward, forwardmove * FRAMETIME, forward);
		VectorAdd(new_origin, forward, new_origin);
	}
	if (sidemove)
	{
		VectorScale(right, sidemove * FRAMETIME, right);
		VectorAdd(new_origin, right, new_origin);
	}

	VectorCopy(new_origin, trace_end);
	trace_end[2] -= 600;

	tr = gi.trace(new_origin, mins, maxs, trace_end, self, MASK_SOLID | MASK_WATER);

	if (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		return true;
	}
	return false;
}

qboolean CheckFall(edict_t* self, short forwardmove, short sidemove, vec3_t angles)
{
	vec3_t	new_origin = { 0 };	//origin after move
	vec3_t	forward, right, up;
	vec3_t	drop_dest = { 0 };	//drop destination
	vec3_t  mins = { 0, 0, 0 }, maxs = { 0, 0, 0 };
	trace_t	tr;

	if (forwardmove == 0 && sidemove == 0)
		return true;

	AngleVectors(angles, forward, right, up);
	VectorNormalize(forward);
	VectorNormalize(right);

	if (forwardmove > 10)
		VectorScale(forward, forwardmove * FRAMETIME, forward);
	if (sidemove > 10)
		VectorScale(right, sidemove * FRAMETIME, right);

	VectorCopy(self->s.origin, new_origin);

	if (forwardmove > 10)
	{
		new_origin[0] += forward[0];
		new_origin[1] += forward[1];
		new_origin[2] += forward[2];
	}
	if (sidemove > 10)
	{
		new_origin[0] += right[0];
		new_origin[1] += right[1];
		new_origin[2] += right[2];
	}

	VectorCopy(new_origin, drop_dest);
	drop_dest[2] -= 150;

	tr = gi.trace(new_origin, mins, maxs, drop_dest, self, MASK_SOLID | MASK_WATER);

	if ((tr.fraction == 1)
		|| (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		self->velocity[0] = 0;
		self->velocity[1] = 0;
		return false;
	}
	return true;
}

void Bot_Wave(edict_t* ent, int i, float time)
{
	if (ent->client->fakedeath > 0)
		return;

	if (ent->health <= 0) /* MrG{DRGN}  was !ent-health >0 */
		return;

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	/* MrG{DRGN} This won't work like this!
	if (ent->client->anim_priority == (ANIM_DEATH || ANIM_PAIN))
		return; */
	if ((ent->client->anim_priority == ANIM_DEATH) || (ent->client->anim_priority == ANIM_PAIN))
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		cprint_botsafe(ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01 - 1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		cprint_botsafe(ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01 - 1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		cprint_botsafe(ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01 - 1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		cprint_botsafe(ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01 - 1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		cprint_botsafe(ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01 - 1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
	ent->client->b_pausetime = level.time + time;
}

static char	BotSayBuff[0x2000];

void Bot_Say(edict_t* ent, qboolean team, char* fmt, ...)
{
	int i;
	/* int		len;
	MrG{DRGN} unused! */
	va_list		argptr;
	edict_t* cl_ent;

	if (!botchat->value)
		return;

	va_start(argptr, fmt);

	vsnprintf(BotSayBuff, sizeof(BotSayBuff), fmt, argptr);
	va_end(argptr);

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, BotSayBuff);

	for (i = 0; i < maxclients->value; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || cl_ent->bot_player)
			continue;

		if (team && !TeamMembers(ent, cl_ent))
			continue;

		gi.cprintf(cl_ent, PRINT_CHAT, BotSayBuff);
	}
}

qboolean Bot_CanJump(edict_t* ent)
{
	trace_t	tr;
	vec3_t	dest = { 0 };

	VectorCopy(ent->s.origin, dest);
	dest[2] += 1;

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_PLAYERSOLID);

	if (!tr.startsolid && (tr.fraction == 1))
		return 1;

	return 0;
}

qboolean Bot_CanStrafe(edict_t* ent, short forwardmove, short sidemove)
{
	trace_t	tr;
	vec3_t	dest = { 0 }, right, forward;

	if (forwardmove == 0 && sidemove == 0)
		return true;

	AngleVectors(ent->s.angles, forward, right, NULL);
	VectorNormalize(forward);
	VectorNormalize(right);

	if (forwardmove > 10)
		VectorScale(forward, forwardmove * FRAMETIME, forward);
	if (sidemove > 10)
		VectorScale(right, sidemove * FRAMETIME, right);

	VectorAdd(ent->s.origin, forward, dest);
	VectorAdd(dest, right, dest);

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_PLAYERSOLID);

	if (!tr.startsolid && (tr.fraction == 1))
		return 1;

	return 0;
}

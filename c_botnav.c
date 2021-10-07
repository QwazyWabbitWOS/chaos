#include "g_local.h"
#include "c_base.h"
#include "c_botnav.h"

/*
Init the note table system
*/
void Bot_InitNodes(void)
{
	int i, l;

	numnodes = -1;

	for (i = 0; i < MAX_NODES; i++)
	{
		nodes[i].origin[0] = 0.0;
		nodes[i].origin[1] = 0.0;
		nodes[i].origin[2] = 0.0;
		nodes[i].flag = 0;
		nodes[i].duckflag = 0;

		for (l = 0; l < MAX_NODES; l++)
		{
			nodes[i].dist[l] = (double)BOT_INFINITY;
		}
	}
}

qboolean Bot_FindNode(edict_t* self, float radius, int flag)
{
	int		i;
	vec3_t	dvec;

	if (flag == -1)	//find all node types
	{
		for (i = 0; i <= numnodes; i++)
		{
			dvec[0] = nodes[i].origin[0] - self->s.origin[0];
			dvec[1] = nodes[i].origin[1] - self->s.origin[1];
			dvec[2] = nodes[i].origin[2] - self->s.origin[2];

			if (VectorLength(dvec) <= radius
				&& visible2(self->s.origin, nodes[i].origin))
				return true;
		}
		return false;
	}
	else	//find special node types
	{
		for (i = 0; i <= numnodes; i++)
		{
			dvec[0] = nodes[i].origin[0] - self->s.origin[0];
			dvec[1] = nodes[i].origin[1] - self->s.origin[1];
			dvec[2] = nodes[i].origin[2] - self->s.origin[2];

			if (VectorLength(dvec) <= radius
				&& visible2(self->s.origin, nodes[i].origin)
				&& nodes[i].flag == flag)
				return true;
		}
		return false;
	}
}

int Bot_FindNodeAtEnt(vec3_t	spot)
{
	int		i, best = -1;
	vec3_t	dvec = { 0, 0, 0 };
	vec_t	bestdist = 180, dist;

	for (i = 0; i <= numnodes; i++)
	{
		dvec[0] = nodes[i].origin[0] - spot[0];
		dvec[1] = nodes[i].origin[1] - spot[1];
		dvec[2] = nodes[i].origin[2] - spot[2];

		dist = VectorLength(dvec);
		if (dist < bestdist)
		{
			if (visible2(spot, nodes[i].origin))
			{
				best = i;
				bestdist = dist;
			}
		}
	}
	return best;
}

int RecalculateCurrentNode(edict_t* ent)
{
	int		dist, bestdist = 200;
	int		i, n, bestnode;
	vec3_t	dvec;

	n = ent->client->b_currentnode;
	bestnode = n;

	for (i = n; i >= 0; i--)
	{
		if (ent->client->b_path[i])
		{
			dvec[0] = nodes[ent->client->b_path[i]].origin[0] - ent->s.origin[0];
			dvec[1] = nodes[ent->client->b_path[i]].origin[1] - ent->s.origin[1];
			dvec[2] = nodes[ent->client->b_path[i]].origin[2] - ent->s.origin[2];

			dist = (int)VectorLength(dvec); /* MrG{DRGN} result is discarded*/
			if ((dist < bestdist) && Bot_CanReachSpotDirectly(ent, nodes[ent->client->b_path[i]].origin))
			{
				bestdist = dist;
				bestnode = i;
			}
		}
	}
	return bestnode;
}

qboolean visible2(vec3_t spot1, vec3_t spot2)
{
	trace_t	trace;
	vec3_t	mins = { 0, 0, 0 };
	vec3_t	maxs = { 0, 0, 0 };

	trace = gi.trace(spot1, mins, maxs, spot2, NULL, MASK_SOLID);

	if (trace.fraction == 1.0)
		return true;
	return false;
}

qboolean visible_node(vec3_t spot1, vec3_t spot2)
{
	trace_t	tr;

	tr = gi.trace(spot1, NULL, NULL, spot2, NULL, MASK_SOLID);

	if (tr.fraction == 1.0 || (tr.ent && (Q_stricmp(tr.ent->classname, "func_door") == 0)))
		return true;
	return false;
}

void Bot_PlaceNode(vec3_t spot, int flag, int duckflag)
{
	if (numnodes < MAX_NODES - 1)
	{
		// don't place nodes in lava/slime
		if (gi.pointcontents(spot) & (CONTENTS_LAVA | CONTENTS_SLIME))
		{
			nprintf(PRINT_HIGH, "Lava/Slime position, node NOT placed!\n");
			return;
		}

		// ok let's place that node
		numnodes++;
		VectorCopy(spot, nodes[numnodes].origin);
		nodes[numnodes].flag = flag;

		if (duckflag == 1)
			nodes[numnodes].duckflag = 1;
		else
			nodes[numnodes].duckflag = 0;

		// print a nice debug message
		if (flag == NORMAL_NODE && duckflag)
			nprintf(PRINT_HIGH, "Normal node %d placed, duckflag on!\n", numnodes);
		else if (flag == NORMAL_NODE && !duckflag)
			nprintf(PRINT_HIGH, "Normal node %d placed!\n", numnodes);
		else if (flag == INAIR_NODE)
			nprintf(PRINT_HIGH, "In-Air node %d placed!\n", numnodes);
		else if (flag == PLAT_NODE)
			nprintf(PRINT_HIGH, "Plat node %d placed!\n", numnodes);
		else if (flag == BUTTON_NODE)
			nprintf(PRINT_HIGH, "Button node %d placed!\n", numnodes);
		else if (flag == TELEPORT_NODE)
			nprintf(PRINT_HIGH, "Teleporter node %d placed!\n", numnodes);
		else if (flag == TRAIN_NODE)
			nprintf(PRINT_HIGH, "Train node %d placed!\n", numnodes);
		else if (flag == LADDER_NODE)
		{
			//edict_t	*arrow;

			nprintf(PRINT_HIGH, "Ladder node %d placed!\n", numnodes);

			/*arrow = G_Spawn();
			VectorCopy (spot, arrow->s.origin);

			arrow->movetype = MOVETYPE_NONE;
			arrow->clipmask = MASK_SHOT;
			arrow->solid = SOLID_NOT;
			arrow->s.effects = 0;
			VectorClear (arrow->mins);
			VectorClear (arrow->maxs);
			arrow->s.modelindex = gi.modelindex ("models/objects/gibs/skull/tris.md2");
			arrow->classname = "debugarrow";
			gi.linkentity (arrow);*/
		}
	}
}

void Bot_CalcNode(edict_t* self, int nindex)
{
	int			i;
	vec3_t		dvec, down, midair, addvect;
	double		dist;
	trace_t		tr;
	vec_t		height;
	vec3_t  mins = { 0, 0, 0 }, maxs = { 0, 0, 0 };

	//go through all nodes
	for (i = 0; i <= numnodes; i++)
	{
		//node == currentnode so dist is 1 (FIXME: 0 better ?)
		if (i == nindex)
		{
			nodes[nindex].dist[i] = 1;
			continue;
		}

		//look if node is visible from current node
		if (visible2(nodes[i].origin, nodes[nindex].origin))
		{
			dvec[0] = nodes[i].origin[0] - nodes[nindex].origin[0];
			dvec[1] = nodes[i].origin[1] - nodes[nindex].origin[1];
			dvec[2] = nodes[i].origin[2] - nodes[nindex].origin[2];

			dist = (double)VectorLength(dvec);

			if (dist > 160) //check if current node is in range
				continue;

			if (gi.pointcontents(nodes[i].origin) & (CONTENTS_WATER)
				|| gi.pointcontents(nodes[nindex].origin) & (CONTENTS_WATER))
			{
				// at least one node is in water so don't check for midair position!

				//connect the two nodes
				nodes[nindex].dist[i] = dist;
				nodes[i].dist[nindex] = dist;

				nprintf(PRINT_HIGH, "Water route!\n");
			}
			else if (nodes[nindex].flag == LADDER_NODE
				|| nodes[i].flag == LADDER_NODE)
			{
				// at least one node is a ladder node so don't check for midair position!

				//connect the two nodes
				nodes[nindex].dist[i] = dist;
				nodes[i].dist[nindex] = dist;

				nprintf(PRINT_HIGH, "Ladder route!\n");
			}
			else if (nodes[nindex].flag == INAIR_NODE && nodes[i].flag == INAIR_NODE)
			{
				// both nodes are in-air nodes...check if we can connect them

				height = nodes[nindex].origin[2] - nodes[i].origin[2];

				if (height > 20)
				{
					//connect only one direction
					nodes[nindex].dist[i] = dist;
				}
				else if (height < -20)
				{
					//connect only one direction
					nodes[i].dist[nindex] = dist;
				}

				//else connect no direction
			}
			else
			{
				//find the mid of the line between the two nodes
				VectorSubtract(nodes[i].origin, nodes[nindex].origin, addvect);
				VectorScale(addvect, 0.5, addvect);
				VectorAdd(nodes[nindex].origin, addvect, midair);

				//find a spot some units below our mid-point
				VectorCopy(midair, down);
				down[2] -= 60;

				//trace down to see if we are on ground
				tr = gi.trace(midair, mins, maxs, down, self, MASK_SOLID);

				if (tr.fraction == 1.0)	//we are not on ground -> midair
				{
					nprintf(PRINT_HIGH, "Midair route!\n");

					height = nodes[nindex].origin[2] - nodes[i].origin[2];

					if (height > 35)
					{
						//connect only one direction
						nodes[nindex].dist[i] = dist;
					}
					else if (height < -35)
					{
						//connect only one direction
						nodes[i].dist[nindex] = dist;
					}
					else
					{
						//connect the two nodes
						nodes[nindex].dist[i] = dist;
						nodes[i].dist[nindex] = dist;
					}
				}
				else //we are on ground
				{
					//connect the two nodes
					nodes[nindex].dist[i] = dist;
					nodes[i].dist[nindex] = dist;
				}
			}
		}
	}
}

int Bot_ShortestPath(int source, int target)
{
	int		i, k, kNew, j;
	double	minDist;

	//clear path buffer
	for (j = 0; j < 100; j++)
		path_buffer[j] = -1;

	//direct path, source=target
	if (source == target)
	{
		path_buffer[0] = target;
		first_pathnode = 0;
		return VALID_PATH;
	}

	// initialize state
	for (i = 0; i < numnodes; i++)
	{
		nodeinfo[i].predecessor = noPredecessor;
		nodeinfo[i].dist = (double)BOT_INFINITY;
		nodeinfo[i].state = tentative;
	}

	// initialize start position
	nodeinfo[source].dist = 0;
	nodeinfo[source].state = permanent;

	// k is working (permanent) node
	k = source;

	do
	{
		// kNew is the tentatively labeled node with the smallest path size
		kNew = BOT_INFINITY;
		minDist = (double)BOT_INFINITY;

		// is there a better path from k
		for (i = 0; i < numnodes; i++)
		{
			double	nodeIdist;
			double	nodeKdist;
			double	distKI;

			distKI = nodes[k].dist[i];
			nodeIdist = nodeinfo[i].dist;
			nodeKdist = nodeinfo[k].dist;

			if ((distKI != BOT_INFINITY) && (nodeinfo[i].state == tentative))
			{
				if ((nodeKdist + distKI) < nodeIdist)
				{
					nodeinfo[i].predecessor = k;
					nodeinfo[i].dist = nodeIdist = nodeKdist + distKI;
				}
			}

			if ((nodeIdist < minDist) && (nodeinfo[i].state == tentative))
			{
				kNew = i;
				minDist = nodeIdist;
			}
		} // end for-i

		// bail out if no path can be found

		if (kNew == BOT_INFINITY)
			return NO_PATH;

		// make that node permanent; there cannot exist a shorter path from source to k
		k = kNew;
		nodeinfo[k].state = permanent;
	} while (k != target);

	// copy path to output array
	i = 0; k = target;
	do
	{
		path_buffer[i++] = k;
		k = nodeinfo[k].predecessor;

		if (k != noPredecessor)
			first_pathnode = i;
	} while (k != noPredecessor && i < 100);

	return VALID_PATH;
}

qboolean Bot_SaveNodes(void)
{
	int		i, j;
	float	dist;
	FILE* output;
	char	file[256];
	const char	nodetable_version[] = "v02";  /* MrG{DRGN} let the compiler figure out the size */
	const char	nodetable_id[] = "CHAOSDM NODE TABLE";	/* MrG{DRGN} let the compiler figure out the size */
	int	dntgvalue = (int)dntg->value; /* MrG{DRGN} safe cast */

	Com_strcpy(file, sizeof file, "./");
	Com_strcat(file, sizeof file, game_dir->string);
	Com_strcat(file, sizeof file, "/nodes/");
	Com_strcat(file, sizeof file, level.mapname);
	Com_strcat(file, sizeof file, ".ntb");

	if ((output = fopen(file, "wb")) == NULL)    /* MrG{DRGN} check the return */
	{
		gi.dprintf("Unable to open file! %s.\n", strerror(errno));
		return false;
	}

	//check1
	fwrite(nodetable_id, sizeof(const char), 19, output);
	fwrite(nodetable_version, sizeof(const char), 4, output);
	fwrite(&numnodes, sizeof(int), 1, output);

	//dynamic node table generation on/off
	fwrite(&dntgvalue, sizeof(int), 1, output);

	for (i = 0; i < numnodes; i++)
	{
		fwrite(&nodes[i].flag, sizeof(int), 1, output);
		fwrite(&nodes[i].duckflag, sizeof(int), 1, output);
		fwrite(&nodes[i].origin, sizeof(vec3_t), 1, output);

		for (j = 0; j < numnodes; j++)
		{
			dist = (float)nodes[i].dist[j];
			fwrite(&dist, sizeof(float), 1, output);
		}
	}
	//check2
	fwrite(nodetable_id, sizeof(const char), 19, output);
	fwrite(nodetable_version, sizeof(const char), 4, output);

	gi.dprintf("%d nodes written to %s\n", numnodes, file);

	fclose(output);
	return true;
}

qboolean Bot_LoadNodes(void)
{
	int		i, j;

	float	dist;
	FILE* input;
	char	file[256];
	const char	nodetable_version[] = "v02";
	const char	nodetable_id[] = "CHAOSDM NODE TABLE";
	char	id_buffer[28] = { 0 };
	char	version_buffer[5] = { 0 };
	int		dntgvalue = 0;
	size_t	num;

	Com_strcpy(file, sizeof file, "./");
	Com_strcat(file, sizeof file, game_dir->string);
	Com_strcat(file, sizeof file, "/nodes/");
	Com_strcat(file, sizeof file, level.mapname);
	Com_strcat(file, sizeof file, ".ntb");

	if ((input = fopen(file, "rb")) == NULL)
	{
		gi.dprintf("Unable to open %s! %s.\n", file, strerror(errno));
		return false;
	}

	//check 1
	num = fread(id_buffer, sizeof(const char), sizeof nodetable_id, input);
	if (!num && !feof(input))
		gi.dprintf("Chaos: %s error reading nodetable_id\n", __func__);
	num = fread(version_buffer, sizeof(const char), sizeof nodetable_version, input);
	if (!num && !feof(input))
		gi.dprintf("Chaos: %s error reading nodetable_version\n", __func__);
	num = fread(&numnodes, sizeof(int), 1, input);
	if (!num && !feof(input))
		gi.dprintf("Chaos: %s error reading numnodes\n", __func__);

	//dynamic node table generation on/off
	num = fread(&dntgvalue, sizeof(int), 1, input);

	if (dntgvalue == 1)
	{
		gi.dprintf("\nDynamic Node Table Generation ON\n");
		gi.cvar_set("dntg", "1");
	}
	else
	{
		gi.dprintf("\nDynamic Node Table Generation OFF\n");
		gi.cvar_set("dntg", "0");
	}

	if (numnodes > MAX_NODES)
	{
		numnodes = 0;
		fclose(input); /* MrG{DRGN} fixed resource leak! */
		return false;
	}

	if ((strcmp(id_buffer, nodetable_id) != 0)) /* MrG{DRGN} simplify */
	{
		fclose(input); /* MrG{DRGN} fixed resource leak! */
		return false;
	}
	if ((strcmp(version_buffer, nodetable_version) != 0))/* MrG{DRGN} simplify */
	{
		fclose(input); /* MrG{DRGN} fixed resource leak! */
		return false;
	}

	for (i = 0; i < numnodes; i++)
	{
		num = fread(&nodes[i].flag, sizeof(int), 1, input);
		if (!num && !feof(input))
			gi.dprintf("Chaos: %s error reading node flags\n", __func__);
		num = fread(&nodes[i].duckflag, sizeof(int), 1, input);
		if (!num && !feof(input))
			gi.dprintf("Chaos: %s error reading node duckflags\n", __func__);
		num = fread(&nodes[i].origin, sizeof(vec3_t), 1, input);
		if (!num && !feof(input))
			gi.dprintf("Chaos: %s error reading node origins\n", __func__);

		for (j = 0; j < numnodes; j++)
		{
			num = fread(&dist, sizeof(float), 1, input);
			nodes[i].dist[j] = (double)dist;
		}
	}

	//check 2
	num = fread(id_buffer, sizeof(const char), sizeof nodetable_id, input);
	if (!num && !feof(input))
		gi.dprintf("Chaos: %s error reading nodetable_id\n", __func__);
	num = fread(version_buffer, sizeof(const char), sizeof nodetable_version, input);
	if (!num && !feof(input))
		gi.dprintf("Chaos: %s error reading nodetable_version\n", __func__);

	if ((strcmp(id_buffer, nodetable_id) != 0))/* MrG{DRGN} simplify */
		return false;
	if ((strcmp(version_buffer, nodetable_version) != 0)) /* MrG{DRGN} simplify */
		return false;

	fclose(input);
	gi.dprintf("%d nodes read from %s\n", numnodes, file);
	return true;
}

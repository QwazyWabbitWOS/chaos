#include "g_local.h"

void ShutOff_Scanner(edict_t* ent)
{
	ent->client->scanneractive = 0;
	ent->client->nextscannercell = level.time;
}

void ShowScanner(edict_t* ent, char* layout)
{
	edict_t* player = g_edicts;
	int     i;
	char    stats[64] = { 0 };
	vec3_t  v = { 0 };

	if (!ent)
	{
		return;
	}

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

void Toggle_Scanner(edict_t* ent)
{
	if (!ent->client)
		return;
	if (ent->health <= 0)
		return;
	/* MrG{DRGN} if you haven't joined a team yet. you can't use the scanner */
	if (ent->client->resp.spectator)
		return;

	if (ent->client->scanneractive <= 0)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] == 0)
		{
			cprint_botsafe(ent, PRINT_HIGH, "You don't have enough cells to run your scanner!\n");
			return;
		}
		cprint_botsafe(ent, PRINT_HIGH, "Scanner ON\n");
		ent->client->scanneractive = 1;
		ent->client->showinventory = 0;
		ent->client->showscores = 0;
	}
	else
	{
		ent->client->scanneractive = 0;
		cprint_botsafe(ent, PRINT_HIGH, "Scanner OFF\n");
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
			cprint_botsafe(ent, PRINT_HIGH, "You don't have enough cells to run your scanner!\n");
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

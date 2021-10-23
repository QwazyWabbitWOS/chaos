#include "g_local.h"
#include "c_botai.h"

/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission(edict_t* ent)
{
	if (!ent->bot_player)
		ent->client->showscores = true;

	VectorCopy(level.intermission_origin, ent->s.origin);
	/* MrG{DRGN}
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8;
	*/
	ent->client->ps.pmove.origin[0] = COORD2SHORT(level.intermission_origin[0]);
	ent->client->ps.pmove.origin[1] = COORD2SHORT(level.intermission_origin[1]);
	ent->client->ps.pmove.origin[2] = COORD2SHORT(level.intermission_origin[2]);

	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = REMOVED_MODEL;
	ent->s.modelindex2 = REMOVED_MODEL;
	ent->s.modelindex3 = REMOVED_MODEL;
	ent->s.modelindex4 = REMOVED_MODEL;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value && !ent->bot_player)
	{
		ent->client->showscores = true;
		DeathmatchScoreboardMessage(ent, NULL);
		gi.unicast(ent, true);
	}
}

void BeginIntermission(edict_t* targ)
{
	int		i, n;
	edict_t* ent, * client;

	if (level.intermissiontime)
		return;		// already activated

	//ZOID
	if ((deathmatch->value) || ctf->value)
		CTFCalcScores();
	//ZOID

	game.autosaved = false;

	// respawn any dead clients
	for (i = 0; i < maxclients->value; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;

		if (client->client && client->client->camera)
		{
			PutClientInServer(client);
			ClientBegin(client);
		}

		if (client->health <= 0)
		{
			if (client->classindex == BOT)
				Bot_Respawn(client);
			else if (client->client && !client->client->camera)
				respawn(client);
		}
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i = 0; i < maxclients->value; i++)
			{
				client = g_edicts + 1 + i;
				if (!client->inuse || !client->client)
					continue;
				// strip players of all keys between units
				for (n = 0; n < MAX_ITEMS; n++)
				{
					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1;		// go immediately to the next level
			BotClearGlobals();
			return;
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find(NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find(ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find(ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy(ent->s.origin, level.intermission_origin);
	VectorCopy(ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i = 0; i < maxclients->value; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission(client);
	}

	BotClearGlobals();
	MaplistMapModeSetup();
}

/*
==================
Collect and sort the scores. Display top 12 players.
Tag killee and killer with appropriate plaques.
==================
*/
void DeathmatchScoreboardMessage(edict_t* ent, edict_t* killer /* MrG{DRGN} can be NULL */)
{
	char	entry[1024];
	char	string[LAYOUT_MAX_LENGTH] = { 0 };
	size_t	stringlength = 0;
	int		i;
	int		j = 0;
	int		k;
	int		n = 0, maxsize = LAYOUT_MAX_LENGTH;
	size_t	len = 0;
	int		sorted[MAX_CLIENTS] = { 0 };
	int		sortedscores[MAX_CLIENTS] = { 0 };
	int		score;
	int		total = 0;
	int		x, y;
	gclient_t* cl;
	edict_t* cl_ent;
	char* tag;

	if (!ent)
		return;

	if (ent->client->showscores || ent->client->showinventory)
		ent->client->scanneractive = 0;

	if (ent->client->menu && ent->client->showscores)
	{
		PMenu_Close(ent);
		ent->client->showscores = true; // turn showscores back on
	}

	if (ent->client->showscores)
	{
		if (ctf->value) {
			CTFScoreboardMessage(ent, killer);
			return;
		}

		// sort the clients by score
		for (i = 0; i < game.maxclients; i++)
		{
			cl_ent = g_edicts + 1 + i;

			if ((!cl_ent->inuse) || (cl_ent->client->camera))
				continue;

			score = game.clients[i].resp.score;
			for (j = 0; j < total; j++)
			{
				if (score > sortedscores[j])
					break;
			}
			for (k = total; k > j; k--)
			{
				sorted[k] = sorted[k - 1];
				sortedscores[k] = sortedscores[k - 1];
			}
			sorted[j] = i;
			sortedscores[j] = score;
			total++;
		}

		// print level name and exit rules
		string[0] = 0;
		stringlength = strlen(string);

		// add the clients in sorted order
		if (total > 12)
			total = 12;

		for (i = 0; i < total; i++)
		{
			cl = &game.clients[sorted[i]];
			cl_ent = g_edicts + 1 + sorted[i];

			x = (i >= 6) ? 160 : 0; // column selection
			y = 32 + 32 * (i % 6); // dogtag is 32 units high (4 lines * 8 high)

			// add a dogtag
			if (cl_ent == ent)
				tag = "tag1"; // victim
			else if (cl_ent == killer)
				tag = "tag2"; // victor
			else
				tag = NULL; // transparent for others

			if (tag) // they're tagged so show the plates
			{
				Com_sprintf(entry, sizeof entry,
					"xv %i yv %i picn %s ", x + 32, y, tag);
				j = (int)strlen(entry);
				if (stringlength + j > sizeof entry)
					break;
				strcpy(string + stringlength, entry); //QW// safe, do not change.
				stringlength += j;
			}

			int time_in = level.framenum - cl->resp.enterframe;

			// send the layout data per player
			Com_sprintf(entry, sizeof entry,
				"client %i %i %i %i %i %i ",
				x, y, sorted[i],
				cl->resp.score,
				cl->ping, time_in / 600);
			j = (int)strlen(entry);
			if (stringlength + j > sizeof entry)
				break;
			strcpy(string + stringlength, entry);  //QW// Can't use Com_strcpy here.
			stringlength += j;
		}
	} // End of regular scoreboard.

	// We actually only care about the first column.
	j = (total % 12) * 32 + 40; // 1 line lower than last plate
	if (total > 6)
		j = 32 * 7 + 8; // this is as low as we go after 6 players

	k = 0; // trapdoor for spectators line.
	n += 16;
	if (maxsize - len > 50) {
		for (i = 0; i < maxclients->value; i++) {
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			// excluding non-spectators and quitters
			if (!cl_ent->inuse || (!cl_ent->client->camera))
				continue;

			if (!k) { // print the spectator header line
				k = 1;
				sprintf(entry, "xv 0 yv %d string2 \"Spectators:\" ", j);
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
				j += 8; // next line
			}

			// compose the CTF style info
			sprintf(entry + strlen(entry),
				"ctf %i %i %i %i %i ",
				(n & 1) ? 160 : 0, // two columns
				j, // y
				i, // client number
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);
			if (maxsize - len > strlen(entry))
			{
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
			}

			if (n & 1)
				j += 8;
			n++;
		}
	}
	// Scanner active ?
	if (ent->client->scanneractive > 0)
		ShowScanner(ent, string);
	gi.WriteByte(svc_layout);
	gi.WriteString(string);

	//DbgPrintf("%s\n", string);
}

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard(edict_t* ent)
{
	DeathmatchScoreboardMessage(ent, ent->enemy);
	gi.unicast(ent, true);
}

/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;
	//ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	//ZOID

	if (!deathmatch->value && !coop->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;

	DeathmatchScoreboard(ent);
}

/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f(edict_t* ent)
{
	//FWP Avoid crashing if ent->classname undefined
	if (!ent->classname)
		return;

	/* this is for backwards compatibility */

	if (ent->classindex == BOT) //MATTHIAS
		return;

	if (deathmatch->value || ctf->value)
	{
		Cmd_Score_f(ent);
		return;
	}

}

//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t* ent)
{
	gitem_t* item;
	int			index, cells = 0; /* MrG{DRGN} initialized */
	int			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	if (!ent->client->ammo_index /* || !ent->client->pers.inventory[ent->client->ammo_index] */)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}

	//
	// armor
	//
	power_armor_type = PowerArmorType(ent);
	if (power_armor_type)
	{
		cells = ent->client->pers.inventory[ITEM_INDEX(it_cells)];// MrG{DRGN} 
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;
		}
	}

	index = ArmorIndex(ent);
	if (power_armor_type && (!index || (level.framenum & 8)))
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex("i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex(index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//

	if (Jet_Active(ent))
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_jet");
		ent->client->ps.stats[STAT_TIMER] = ent->client->jet_remaining / 10;
	}
	else if (ent->client->kamikazetime > 0)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_kamikaze");
		ent->client->ps.stats[STAT_TIMER] = ent->client->kamikazetime / 10;
	}
	else if (ent->client->invisible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_invis");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invisible_framenum - level.framenum) / 10;
	}
	else if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum) / 10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_invulnerability");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum) / 10;
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_envirosuit");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum) / 10;
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_rebreather");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum) / 10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores || ent->client->scanneractive > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->resp.helpchanged && (level.framenum & 8))
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
	else if ((ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91)
		&& ent->client->pers.weapon)
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex(ent->client->pers.weapon->icon);
	else
		ent->client->ps.stats[STAT_HELPICON] = 0;

	//ZOID
	SetCTFStats(ent);
	//ZOID
}

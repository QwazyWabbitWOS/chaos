#include "g_local.h"
#include "m_player.h"

char* ClientTeam(edict_t* ent)
{
	char* p;
	static char	value[512] = { 0 };

	value[0] = 0;

	if (!ent->client)
		return value;
	
	Com_strcpy(value, sizeof(value), Info_ValueForKey(ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam(edict_t* ent1, edict_t* ent2)
{
	char	ent1Team[512];
	char	ent2Team[512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	
	Com_strcpy(ent1Team, sizeof(ent1Team), ClientTeam(ent1));
	Com_strcpy(ent2Team, sizeof(ent2Team), ClientTeam(ent2));

	if (Q_stricmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}

void SelectNextItem(edict_t* ent, int itflags)
{
	gclient_t* cl;
	int			i, index;
	gitem_t* it;

	cl = ent->client;

	//Matthias
	if (cl->menu)
	{
		PMenu_Next(ent);
		return;
	}
	else if (cl->camera)
	{
		CamNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (cl->pers.selected_item + i) % MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem(edict_t* ent, int itflags)
{
	gclient_t* cl;
	int			i, index;
	gitem_t* it;

	cl = ent->client;

	//Matthias
	if (cl->menu)
	{
		PMenu_Prev(ent);
		return;
	}
	else if (cl->camera)
	{
		CamPrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i) % MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem(edict_t* ent)
{
	gclient_t* cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem(ent, -1);
}

//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(edict_t* ent)
{
	char* name;
	gitem_t* it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t* it_ent;

	if (!sv_cheats->value)
	{
		cprint_botsafe(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
		{
			ent->health = atoi(gi.argv(2));
			ent->health = ent->health < 1 ? 1 : ent->health; /* MrG{DRGN} Give < -1 health, and increasing it to > 0 without closing the console breaks the player state.*/
		}
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			if (it == it_machinegun
				|| it == it_shotgun
				|| it == it_chaingun)
				continue;

			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			if (it == it_bullets)
				continue;

			Add_Ammo(ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t* info;

		it = FindItemByClassindex(AR_JACKET);
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItemByClassindex(AR_COMBAT);
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItemByClassindex(AR_BODY);
		info = (gitem_armor_t*)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItemByClassindex(AR_POWER_SHIELD);
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem(it_ent, it);
		Touch_Item(it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR | IT_WEAPON | IT_AMMO))
				continue;
			if (it == it_flag_red  //MATTHIAS /* MrG{DRGN} */
				|| it == it_flag_blue
				|| it == it_tech1
				|| it == it_tech2
				|| it == it_tech3
				|| it == it_tech4
				|| it == it_machinegun
				|| it == it_shotgun
				|| it == it_chaingun
				|| it == it_bullets)
				continue;

			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem(name);

	if (it == it_flag_red  //MATTHIAS /* MrG{DRGN} */
		|| it == it_flag_blue
		|| it == it_machinegun
		|| it == it_shotgun
		|| it == it_chaingun
		|| it == it_bullets)
		return;

	if (!it)
	{
		name = gi.argv(1);
		it = FindItem(name);
		if (!it)
		{
			gi.dprintf("unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.dprintf("non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem(it_ent, it);
		Touch_Item(it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(edict_t* ent)
{
	char* msg;

	if (!sv_cheats->value)
	{
		cprint_botsafe(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	cprint_botsafe(ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(edict_t* ent)
{
	char* msg;

	if (!sv_cheats->value)
	{
		cprint_botsafe(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	cprint_botsafe(ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(edict_t* ent)
{
	char* msg;

	if (!sv_cheats->value)
	{
		cprint_botsafe(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	cprint_botsafe(ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f(edict_t* ent)
{
	int			index;
	gitem_t* it;
	char* s;

	if (ent->client->fakedeath > 0)
		return;

	s = gi.args();
	it = FindItem(s);
	if (!it)
	{
		cprint_botsafe(ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		cprint_botsafe(ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		cprint_botsafe(ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use(ent, it);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f(edict_t* ent)
{
	int			index;
	gitem_t* it;
	char* s;

	//ZOID--special case for tech powerups
	if (Q_stricmp(gi.args(), "tech") == 0 && (it = CTFWhat_Tech(ent)) != NULL)
	{
		it->drop(ent, it);
		return;
	}
	//ZOID

	if (Q_stricmp(gi.args(), "flag") == 0)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Red Flag"))])
		{
			it = FindItem("Red Flag");
			it->drop(ent, it);
			return;
		}
		if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Blue Flag"))])
		{
			it = FindItem("Blue Flag");
			it->drop(ent, it);
			return;
		}
	}

	s = gi.args();
	it = FindItem(s);

	if (!it)
	{
		cprint_botsafe(ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}

	if (!it->drop)
	{
		cprint_botsafe(ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		cprint_botsafe(ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop(ent, it);
}

/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f(edict_t* ent)
{
	int			i;
	gclient_t* cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	//ZOID
	if (ent->client->menu) {
		PMenu_Close(ent);
		return;
	}
	//ZOID

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	//ZOID
	if (ctf->value && cl->resp.ctf_team == CTF_NOTEAM) {
		CTFOpenJoinMenu(ent);
		return;
	}
	//ZOID

	cl->showinventory = true;

	gi.WriteByte(svc_inventory);
	for (i = 0; i < MAX_ITEMS; i++)
	{
		gi.WriteShort(cl->pers.inventory[i]);
	}
	gi.unicast(ent, true);

	cl->scanneractive = 0;	//MATTHIAS
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f(edict_t* ent)
{
	gitem_t* it;

	//ZOID
	if (ent->client->menu) {
		PMenu_Select(ent);
		return;
	}
	//ZOID

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == -1)
	{
		cprint_botsafe(ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		cprint_botsafe(ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use(ent, it);
}

//ZOID
/*
=================
Cmd_LastWeap_f
=================
*/
void Cmd_LastWeap_f(edict_t* ent)
{
	gclient_t* cl;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	cl->pers.lastweapon->use(ent, cl->pers.lastweapon);
}
//ZOID

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f(edict_t* ent)
{
	gclient_t* cl;
	int			i, index;
	gitem_t* it;
	int			selected_weapon;

	if (ent->client->fakedeath > 0)
		return;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (selected_weapon + i) % MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use(ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f(edict_t* ent)
{
	gclient_t* cl;
	int			i, index;
	gitem_t* it;
	int			selected_weapon;

	if (ent->client->fakedeath > 0)
		return;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i) % MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use(ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f(edict_t* ent)
{
	gclient_t* cl;
	int			index;
	gitem_t* it;

	if (ent->client->fakedeath > 0)
		return;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (!(it->flags & IT_WEAPON))
		return;
	it->use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f(edict_t* ent)
{
	gitem_t* it;

	if (!ent->client->camera)
	{
		ValidateSelectedItem(ent);

		if (ent->client->pers.selected_item == -1)
		{
			cprint_botsafe(ent, PRINT_HIGH, "No item to drop.\n");
			return;
		}

		it = &itemlist[ent->client->pers.selected_item];
		if (!it->drop)
		{
			cprint_botsafe(ent, PRINT_HIGH, "Item is not dropable.\n");
			return;
		}
		it->drop(ent, it);
	}
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(edict_t* ent)
{
	//ZOID
	if (ent->solid == SOLID_NOT)
		return;
	//ZOID

	/*
	 * Fix for invincible bug
	 * Major
	 */

	 /* MrG{DRGN} replaced with integer comparision
		if (ent->classindex == CAMPLAYER)
		 if (!Q_stricmp (ent->classname, "camera") */
	if (ent->classindex == CAMPLAYER || ent->client->camera)
		return;
	/*
	 * End Fix
	 */

	if ((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die(ent, ent, ent, 100000, vec3_origin);
	// don't even bother waiting for death frames
	ent->deadflag = DEAD_DEAD;
	respawn(ent);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f(edict_t* ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
	//ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	//ZOID
}

int PlayerSort(void const* a, void const* b)
{
	int		anum, bnum;

	anum = *(int*)a;
	bnum = *(int*)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f(edict_t* ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280] = { 0 };
	int		index[256] = { 0 };

	count = 0;
	for (i = 0; i < maxclients->value; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}



	// sort by frags
	qsort(index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0; i < count; i++)
	{
		Com_sprintf(small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen(small) + strlen(large) > sizeof(large) - 100)
		{	// can't print all of them in one packet
			
			Com_strcat(large, sizeof(large), "...\n");
			break;
		}
		
		Com_strcat(large, sizeof(large), small);

	}

	cprint_botsafe(ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f(edict_t* ent)
{
	int		i;

	if (ent->client->fakedeath > 0)
		return;

	i = atoi(gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
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
}

/* MrG{DRGN} */
// Mute spammy players.
qboolean CheckFlood(edict_t* ent)
{
	int	i;

	gclient_t* cl;

	if (flood_msgs->value) {
		cl = ent->client;

		if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
			return true;
		}

		i = cl->flood_whenhead - flood_msgs->value + 1;

		if (i < 0)
		{
			i = ((int)sizeof(cl->flood_when) / (int)sizeof(cl->flood_when[0])) + i;
		}

		if ((size_t)cl->flood_when[i] && level.time - cl->flood_when[i] < flood_persecond->value)
		{
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
			return true;
		}

		cl->flood_whenhead = (((size_t)cl->flood_whenhead + 1) % (sizeof(cl->flood_when) / sizeof(cl->flood_when[0])));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}
	return false;
}


/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(edict_t* ent, qboolean team, qboolean arg0)
{
	int		j;
	edict_t* other;
	char* p;
	char	text[2048];

	if (gi.argc() < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf(text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		
		Com_strcat(text, sizeof(text), gi.argv(0));
		Com_strcat(text, sizeof(text), " ");
		Com_strcat(text, sizeof(text), gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p) - 1] = 0;
		}			  		
		Com_strcat(text, sizeof(text), p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;
	
	Com_strcat(text, sizeof(text), "\n");

	if (CheckFlood(ent))/* MrG{DRGN} */
		return;

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (Q_stricmp(other->classname, "bot") == 0)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

/*
=================
Process normal client commands
=================
*/
void ClientCommand(edict_t* ent)
{
	char* cmd;

	/* MrG{DRGN} check if there's even an ent first!
	if (!ent->client || !ent->classname) // FWP Check ent->classname too
	*/
	if (!ent)
		return;
	

	cmd = gi.argv(0);

	if (Q_stricmp(cmd, "players") == 0)
	{
		Cmd_Players_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "say") == 0)
	{
		Cmd_Say_f(ent, false, false);
		return;
	}
	if (Q_stricmp(cmd, "say_team") == 0 || Q_stricmp(cmd, "steam") == 0)
	{
		/* MrG{DRGN} Spectators don't have teams and shouldn't be spamming macros */
		if (ent->solid == SOLID_NOT || ent->solid == SOLID_TRIGGER)
			return;
		
		CTFSay_Team(ent, gi.args());
		return;
	}
	if (Q_stricmp(cmd, "score") == 0)
	{
		Cmd_Score_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "help") == 0)
	{
		Cmd_Help_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp(cmd, "use") == 0)
		Cmd_Use_f(ent);
	else if (Q_stricmp(cmd, "drop") == 0)
	{
		/* MrG{DRGN} tech drop prevention */
		if ((Q_stricmp(gi.args(), "tech") == 0) && (!drop_tech->value))
		{
			cprint_botsafe(ent, PRINT_HIGH, "Tech drop is disabled by the admin!\n");
			return;
		}
		else
			Cmd_Drop_f(ent);
	}
	else if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if (Q_stricmp(cmd, "inven") == 0)
		Cmd_Inven_f(ent);
	else if (Q_stricmp(cmd, "invnext") == 0)
		SelectNextItem(ent, -1);
	else if (Q_stricmp(cmd, "invprev") == 0)
		SelectPrevItem(ent, -1);
	else if (Q_stricmp(cmd, "invnexta") == 0)
		SelectNextItem(ent, IT_AMMO);
	else if (Q_stricmp(cmd, "invpreva") == 0)
		SelectPrevItem(ent, IT_AMMO);
	else if (Q_stricmp(cmd, "invnextw") == 0)
		SelectNextItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invprevw") == 0)
		SelectPrevItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invnextp") == 0)
		SelectNextItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invprevp") == 0)
		SelectPrevItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invuse") == 0)
		Cmd_InvUse_f(ent);
	else if (Q_stricmp(cmd, "invdrop") == 0)
		Cmd_InvDrop_f(ent);
	else if (Q_stricmp(cmd, "weapprev") == 0)
		Cmd_WeapPrev_f(ent);
	else if (Q_stricmp(cmd, "weapnext") == 0)
		Cmd_WeapNext_f(ent);
	else if (Q_stricmp(cmd, "weaplast") == 0)
		Cmd_WeapLast_f(ent);
	else if (Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if (Q_stricmp(cmd, "putaway") == 0)
		Cmd_PutAway_f(ent);
	else if (Q_stricmp(cmd, "wave") == 0)
		Cmd_Wave_f(ent);
	else if (Q_stricmp(cmd, "team") == 0)
		CTFTeam_f(ent);
	else if (Q_stricmp(cmd, "id") == 0)
		CTFID_f(ent);
	else if (Q_stricmp(cmd, "yes") == 0) 
		CTFVoteYes(ent);	
	else if (Q_stricmp(cmd, "no") == 0)
		CTFVoteNo(ent);		
	else if (Q_stricmp(cmd, "ready") == 0) 
		CTFReady(ent);		
	else if (Q_stricmp(cmd, "notready") == 0)
		CTFNotReady(ent);	
	else if (Q_stricmp(cmd, "ghost") == 0)
		CTFGhost(ent);	 	
	else if (Q_stricmp(cmd, "admin") == 0)
		CTFAdmin(ent);
	else if (Q_stricmp(cmd, "stats") == 0)
		CTFStats(ent);
	else if (Q_stricmp(cmd, "warp") == 0) 
		CTFWarp(ent);
	else if (Q_stricmp(cmd, "boot") == 0) 
		CTFBoot(ent);
	else
		ClientCommand2(ent);
}

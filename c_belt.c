#include "g_local.h"
#include "c_belt.h"

void Cmd_Belt_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't AG belt! */
		return;

	if (ent->client->fakedeath > 0)
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->beltactive > 0)
	{
		cprint_botsafe(ent, PRINT_HIGH, "Anti gravity belt OFF\n");
		ent->client->beltactive = 0;
		ent->client->nextbeltcell = level.time;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] <= 0)
		{
			cprint_botsafe(ent, PRINT_HIGH, "You don't have enough cells to run your anti gravity belt!\n");
			ent->client->beltactive = 0;
			ent->client->nextbeltcell = level.time;
		}
		else
		{
			cprint_botsafe(ent, PRINT_HIGH, "Anti gravity belt ON\n");
			ent->client->beltactive = 1;
			ent->client->nextbeltcell = level.time + 2;
		}
	}
}

void Belt_Think(edict_t* ent)
{
	gclient_t* client;

	if (ent->health <= 0)
		return;
	client = ent->client;

	if ((level.time > client->nextbeltcell) && (client->beltactive > 0))
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] <= 0)
		{
			cprint_botsafe(ent, PRINT_HIGH, "You don't have enough cells to run your anti gravity belt!\n");
			client->beltactive = 0;
			client->nextbeltcell = level.time;
		}
		else
		{
			ent->client->pers.inventory[ITEM_INDEX(it_cells)]--;
			client->nextbeltcell = level.time + 2;
		}
	}
}
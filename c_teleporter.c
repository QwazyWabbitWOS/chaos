#include "g_local.h"

void ShutOff_teleporter(edict_t* ent)
{
	G_FreeEdict(ent->client->teleporter);
	ent->client->teleporter = NULL;
}

void Teleport_Think(edict_t* ent)
{
	ent->s.frame += 1;
	if (ent->s.frame > 9)
		ent->s.frame = 0;

	ent->nextthink = level.time + FRAMETIME;
}

void Teleport(edict_t* ent)
{
	edict_t* telep = NULL;
	vec3_t spawn_origin = { 0 };

	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->client->teleporter)	//teleport
	{
		int i; // MrG{DRGN}
		if (ctf->value)
			CTFDeadDropFlag(ent);

		telep = ent->client->teleporter;

		VectorCopy(telep->s.origin, spawn_origin);
		spawn_origin[2] += 9;

		for (i = 0; i < 3; i++) {
			ent->client->ps.pmove.origin[i] = COORD2SHORT(spawn_origin[i]);
		}

		ent->s.event = EV_ITEM_RESPAWN;
		spawn_origin[2] += 1;
		VectorCopy(spawn_origin, ent->s.origin);

		if (!KillBox(ent))
		{	// couldn't spawn in?
		}

		gi.sound(telep, CHAN_VOICE, gi.soundindex("misc/selfteleport.wav"), 1, ATTN_NORM, 0);

		//kill teleport point
		telep->s.event = EV_ITEM_RESPAWN; // Make an item respawn effect ! Looks cool ! :)
		G_FreeEdict(telep);
		ent->client->teleporter = NULL;
	}
	else	//create teleporter
	{
		telep = G_Spawn();
		telep->owner = ent;
		telep->solid = SOLID_BBOX;
		telep->classname = "selfteleporter";
		telep->classindex = SELFTELEPORTER;
		telep->s.event = EV_ITEM_RESPAWN;
		telep->s.renderfx = RF_TRANSLUCENT;
		telep->s.effects |= EF_FLAG2;
		telep->clipmask = MASK_SHOT;
		VectorClear(telep->mins);
		VectorClear(telep->maxs);
		/*	telep->owner = ent; redundant reassignment! */

		telep->movetype = MOVETYPE_FLYMISSILE;
		telep->model = "models/objects/selftp/tris.md2";
		gi.setmodel(telep, telep->model);
		VectorCopy(ent->s.origin, telep->s.origin);
		telep->s.origin[2] -= 10;
		telep->think = Teleport_Think;
		telep->nextthink = level.time + FRAMETIME;

		gi.linkentity(telep);
		ent->client->teleporter = telep;

		gi.sound(telep, CHAN_VOICE, gi.soundindex("misc/placetelep.wav"), 1, ATTN_NORM, 0);
	}
}

void Cmd_Teleport_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't teleport! */
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->fakedeath > 0)
		return;
	if (ent->client->teleporter)	//teleport
	{
		Teleport(ent);
	}
	else	//create teleporter
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] < 100)
		{
			cprint_botsafe(ent, PRINT_HIGH, "You need 100 cells to place a self teleporter!\n");
		}
		else
		{
			ent->client->pers.inventory[ITEM_INDEX(it_cells)] -= 100;
			Teleport(ent);
			cprint_botsafe(ent, PRINT_HIGH, "Self Teleporter placed! Use cmd teleport again to use it.\n");
		}
	}
}

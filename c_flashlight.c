#include "g_local.h"
#include "c_flashlight.h"


void Shutoff_Flashlight(edict_t* ent)
{
	ent->client->flashlightactive = false;
	if (ent->client->flashlight)
	{
		ent->client->flashlight->think = G_FreeEdict;
		G_FreeEdict(ent->client->flashlight);
	}
}

void FlashLightThink(edict_t* ent)
{
	vec3_t start, end, endp, offset = { 0 };
	vec3_t forward, right, up;
	trace_t tr;

	if (!ent)
		return;

	if (!ent->owner ||
		!ent->owner->client)
		return;

	AngleVectors(ent->owner->client->v_angle, forward, right, up);

	VectorSet(offset, 24, 6, ent->owner->viewheight - 7.0F);
	G_ProjectSource(ent->owner->s.origin, offset, forward, right, start);
	VectorMA(start, 8192, forward, end);

	tr = gi.trace(start, NULL, NULL, end, ent->owner, CONTENTS_SOLID);

	if ((tr.fraction != 1) || tr.startsolid)
	{
		VectorMA(tr.endpos, -10, forward, endp);
		VectorCopy(endp, tr.endpos);
	}

	vectoangles(tr.plane.normal, ent->s.angles);
	VectorCopy(tr.endpos, ent->s.origin);

	gi.linkentity(ent);
	ent->nextthink = level.time + FRAMETIME;
}

void Cmd_Flashlight_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't use the flashlight! */
		return;

	if (ent->client->fakedeath > 0)
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->flashlightactive)
	{
		ent->client->flashlightactive = false;
		if (ent->client->flashlight)
			ent->client->flashlight->think = G_FreeEdict;
		cprint_botsafe(ent, PRINT_HIGH, "Flashlight OFF\n");
	}
	else
	{
		vec3_t  start, forward, right, end = { 0 };

		ent->client->flashlightactive = true;

		AngleVectors(ent->client->v_angle, forward, right, NULL);

		VectorSet(end, 100, 0, 0);
		G_ProjectSource(ent->s.origin, end, forward, right, start);

		ent->client->flashlight = G_Spawn();
		ent->client->flashlight->think = FlashLightThink;
		ent->client->flashlight->nextthink = level.time + FRAMETIME;
		ent->client->flashlight->s.effects = EF_HYPERBLASTER;
		ent->client->flashlight->s.modelindex = gi.modelindex("models/objects/dummy/tris.md2");
		ent->client->flashlight->solid = SOLID_NOT;
		ent->client->flashlight->owner = ent;
		ent->client->flashlight->classname = "flashlight";
		ent->client->flashlight->classindex = FLASHLIGHT;
		ent->client->flashlight->movetype = MOVETYPE_NOCLIP;
		ent->client->flashlight->clipmask = MASK_SHOT;
		VectorCopy(end, ent->client->flashlight->s.origin);

		gi.linkentity(ent->client->flashlight);

		cprint_botsafe(ent, PRINT_HIGH, "Flashlight ON\n");
	}
}
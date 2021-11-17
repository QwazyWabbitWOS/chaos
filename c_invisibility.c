#include "g_local.h"

void InvisibilityInitVars(void)
{
	ban_invisibility = gi.cvar("ban_invisibility", "0", CVAR_LATCH);
}

void Use_Invisibility(edict_t* ent, const gitem_t* item)
{
	if (!ent || !item)
	{
		return;
	}

	ValidateSelectedItem(ent);

	ent->client->pers.inventory[ITEM_INDEX(item)]--;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/invis.wav"), 0.8F, ATTN_NORM, 0);
	ent->client->invisible = 1;
	ent->s.modelindex = REMOVED_MODEL;

	if (ent->client->invisible_framenum > level.framenum)
		ent->client->invisible_framenum += 300;
	else
		ent->client->invisible_framenum = level.framenum + 300.0F;

	ent->s.event = EV_PLAYER_TELEPORT;
}

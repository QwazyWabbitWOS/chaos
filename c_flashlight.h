#ifndef C_FLASHLIGHT_H
#define C_FLASHLIGHT_H

// MrG{DRGN} never in game, but always posessed!
gitem_t* it_flashlight;

void ShutOff_Flashlight(edict_t* ent);
void FlashLightThink(edict_t* ent);
void Cmd_Flashlight_f(edict_t* ent);

#endif

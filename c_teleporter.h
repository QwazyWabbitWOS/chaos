#pragma once

void ShutOff_teleporter(edict_t* ent);

// MrG{DRGN} never in game, but always possessed!
gitem_t* it_teleporter;

void Teleport_Think(edict_t* ent);
void Teleport(edict_t* ent);
void Cmd_Teleport_f(edict_t* ent);
